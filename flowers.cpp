#include <Arduino.h>

#include <Wire/Wire.h>
#include <stdio.h>
#include <stdarg.h>
#include "User_Setup.h"

#include <LiquidCrystal_I2C/LiquidCrystal_I2C.cpp>
#include <OneWire/OneWire.cpp>
#include <Arduino-Temperature-Control-Library/DallasTemperature.cpp>

#define DEBUG 1

#include "clog.cpp"

#include "i2c.cpp"
#include "actual.cpp"

bool isDay, needUpdateTemp = false;

float temp;

u8 statusPump, statusLight, statusWaterLevel, statusCool;

u16 humidity, perDetHum[3], counter, alh, alt, aht;

u16 pumpCounterTimer, coolCounterTimer, coolTime[8];

OneWire oneWire(PIN_THERMAL);

DallasTemperature td(&oneWire);

LiquidCrystal_I2C lcd(0x3F, 20, 4);  //  Объявляем  объект библиотеки, указывая параметры дисплея

Time cTime{};

u16 minutes() {
  cTime = actualClockTime();
  return (cTime.m) + (cTime.h) * 60;
}

u16 hhmm2min(u8 hh, u8 mm) {
  return mm + hh * 60;
}

void print(char *format, ...) {
  va_list args;
  char buffer[24];
  va_start(args, format);
  vsnprintf(buffer, 24, format, args);
  lcd.print(buffer);
  va_end(args);
}

void readTemperature(bool isDummy, bool initReadReal) {
  if (isDummy) {
    delay(1000);
    temp = 21.0;
    return;
  }
  td.requestTemperatures();
  float t = td.getTempCByIndex(0);
  if (t > -30.0) {
    temp = t;
    needUpdateTemp = false;
  } else if (initReadReal) {
    lcd.setCursor(1, 0);
    print("Read t, wait...");
    u8 max = 0;
    while (t < -100 && max++ < 180) {
      td.requestTemperatures();
      t = td.getTempCByIndex(0);
    }
    if (t < -30.0) {
      temp = 21.0;
      needUpdateTemp = true;
    } else {
      temp = t;
    }
  } else {
    delay(1000);
    needUpdateTemp = true;
  }
  clog("Read temp: %li, %li", round(temp*10), round(t*10));
  //temp = ((data[1] << 8) | data[0]) * 0.0625; // NOLINT(hicpp-signed-bitwise)
}

void coolOff() {
  digitalWrite(PIN_COOL, LOW);
  statusCool = STATUS_OFF;
}

void coolOn(bool isAlarm) {
  if (isAlarm && statusCool != STATUS_ALARM) {
    aht++;
  }
  digitalWrite(PIN_COOL, HIGH);
  statusCool = isAlarm ? STATUS_ALARM : STATUS_ON;
}

void lightOff() {
  digitalWrite(PIN_LIGHT, HIGH);
  statusLight = STATUS_OFF;
}

void lightOn(bool isAlarm) {
  if (isAlarm && statusLight != STATUS_ALARM) {
    alt++;
  }

  digitalWrite(PIN_LIGHT, LOW);
  statusLight = isAlarm ? STATUS_ALARM : STATUS_ON;
}

void pumpOff() {
  digitalWrite(PIN_PUMP, HIGH);
  digitalWrite(PIN_POWER_WATER_LEVEL, LOW);
  statusPump = STATUS_OFF;
  statusWaterLevel = STATUS_OFF;
}

void pumpOn(bool isAlarm) {
  digitalWrite(PIN_POWER_WATER_LEVEL, HIGH);
  delay(50);
  u16 wl = analogRead(PIN_WATER_LEVEL);
  statusWaterLevel = wl > WATER_FULL ? STATUS_ON : STATUS_OFF;

  if (statusWaterLevel) {
    clog("Bottle full, prevent start pump: %d", wl);
    pumpOff();
    return;
  }

  if (isAlarm) {
    alh++;
  }

  digitalWrite(PIN_PUMP, LOW);
  statusPump = isAlarm ? STATUS_ALARM : STATUS_ON;

  u8 alarmTime = 300;
  if (statusPump == STATUS_ALARM) {
    alarmTime = 90;
  }
  pumpCounterTimer = (alarmTime + counter) % COUNTER_MAX;
}

void pumpAlarm() {
  if (!statusPump) {
    return;
  }

  u16 wl = analogRead(PIN_WATER_LEVEL);
  statusWaterLevel = wl > WATER_FULL ? STATUS_ON : STATUS_OFF;
  clog("WL: %i x %u", wl, statusWaterLevel);

  if (statusWaterLevel || pumpCounterTimer == counter) {
    pumpOff();
  }
}

void normalLight() {
  if (isDay) {
    lightOn(false);
  } else {
    lightOff();
  }
}

void tLowAlarm() {
  if (temp > TEMP_LOW) {
    if (statusLight != STATUS_ALARM) {
      return;
    }
    if (temp < TEMP_MIN) {
      return;
    }
    normalLight();
    return;
  }
  if (statusLight) {
    if (statusCool) {
      coolOff();
      lightOn(true);
    }
    return;
  }/*
  if (!statusCool) {
    coolOn(true);
    coolCounterTimer = (counter + 180) % COUNTER_MAX;
    return;
  }*/
//  if (counter == coolCounterTimer) {
//    coolOff();
    lightOn(true);
//  }
}

void tHighAlarm() {
  if (temp < TEMP_HIGH) {
    if (temp > TEMP_MAX) {
      return;
    }
    if (statusCool != STATUS_ALARM) {
      return;
    }
    coolOff();
    normalLight();
    return;
  }
  if (statusLight) {
    lightOff();
  }
  if (statusCool != STATUS_ALARM) {
    coolOn(true);
  }
}

void humidityLowAlarm() {
  digitalWrite(PIN_POWER_HUMIDITY, HIGH);
  delay(75);
  u16 tmp, hMin;
  // read detector 0 (less then 100 is disabled)
  tmp = HUMIDITY_TOP - analogRead(PIN_HUMIDITY_0);
  perDetHum[0] = tmp < HUMIDITY_DISABLE ? HUMIDITY_MIN : tmp;
  //
  tmp = HUMIDITY_TOP - analogRead(PIN_HUMIDITY_1);
  perDetHum[1] = tmp < HUMIDITY_DISABLE ? HUMIDITY_MIN : tmp;
  //
  tmp = HUMIDITY_TOP - analogRead(PIN_HUMIDITY_2);
  perDetHum[2] = tmp < HUMIDITY_DISABLE ? HUMIDITY_MIN : tmp;
  //
  hMin = min(min(perDetHum[0], perDetHum[1]), perDetHum[2]);
  humidity = floor((double)(perDetHum[0] + perDetHum[1] + perDetHum[2]) / 3);
  digitalWrite(PIN_POWER_HUMIDITY, LOW);

  // если почему-то минимальное значение и среднее разбежались на столько,
  // что полив вроде как не нужен для 2/3, а для 1/3 очень критичен, включаем дежурный полив
  if ((hMin <= HUMIDITY_CRIT && statusLight) || humidity < HUMIDITY_CRIT) {
    pumpOn(true);
  } else if (humidity < HUMIDITY_MIN && isDay) { // днём можно поливать, если влажность низкая
    pumpOn(false);
  }
}

bool isAlarm() {
  return statusPump == STATUS_ALARM || statusCool == STATUS_ALARM || statusLight == STATUS_ALARM;
}

void scheduler(u16 now) {
  if (
      (coolTime[0] <= now && now <= coolTime[1]) ||
      (coolTime[2] <= now && now <= coolTime[3]) ||
      (coolTime[4] <= now && now <= coolTime[5]) ||
      (coolTime[6] <= now && now <= coolTime[7])
      ) {
    if (!statusCool) {
      coolOn(false);
    }
  } else {
    if (statusCool == STATUS_ON) {
      coolOff();
    }
  }

  if (TIME_START_DAY <= now && now <= TIME_END_DAY) {
    if (!statusLight) {
      lightOn(false);
    }
  } else {
    if (statusLight == STATUS_ON) {
      lightOff();
    }
  }
}

char STATUS_MAP[3] = {'0', '1', 'A'};

void printStatus(u16 now) {
  // |01234567890123456789|
  // |T: 20   18:08:36 PLC|
  // |H: 123 456 789   01A|
  // |Ha: 67%   ALH: 10000|
  // |ALT:25600 AHT:     0|
  // |01234567890123456789|
  clog("Habs: %u %u %u; Temp: %ld, minutes: %d", perDetHum[0], perDetHum[1], perDetHum[2], round(temp), now);
  Serial.println("---------------------------------------");
  clog("T: %02ld   %02u:%02u:%02u PLC", round(temp), cTime.h, cTime.m, cTime.s);
  clog("H: %03u %03u %03u   %c%c%c", perDetHum[0], perDetHum[1], perDetHum[2],
       STATUS_MAP[statusPump], STATUS_MAP[statusLight], STATUS_MAP[statusCool]);
  clog("Ha: %03u   ALH: %5u", humidity, alh);
  clog("ALT:%5u AHT: %5u", alt, aht);
  Serial.println("---------------------------------------");
  #if LCD_ON
  lcd.setCursor(0, 0);
  print("T: %02ld.%ld %02u:%02u:%02u PLC",
      lround((double)temp),
      (lround((double)temp * 10) % 10),
      cTime.h, cTime.m, cTime.s);
  lcd.setCursor(0, 1);
  print("H: %03u %03u %03u   %c%c%c", perDetHum[0], perDetHum[1], perDetHum[2],
      STATUS_MAP[statusPump], STATUS_MAP[statusLight], STATUS_MAP[statusCool]);
  lcd.setCursor(0, 2);
  print("Ha: %03u   ALH: %5u", humidity, alh);
  lcd.setCursor(0, 3);
  print("ALT:%5u AHT: %5u", alt, aht);
  #endif
}

bool customChange() {
  return false;/*
  if (HIGH == digitalRead(PIN_PUMP_REVERSE)) {
    if (statusPump) {
      pumpOff();
    } else {
      pumpOn(false);
    }
    return true;
  }
  if (HIGH == digitalRead(PIN_LIGHT_REVERSE)) {
    if (statusLight) {
      lightOff();
    } else {
      lightOn(false);
    }
    return true;
  }
  if (HIGH == digitalRead(PIN_COOL_REVERSE)) {
    if (statusPump) {
      coolOff();
    } else {
      coolOn(false);
    }
    return true;
  }
  return false;*/
}

void setup() {
  Serial.begin(9600);
  //pinMode(PIN_THERMAL, INPUT); // 3
  td.begin();
  //
  /*pinMode(PIN_PUMP_REVERSE, INPUT);
  pinMode(PIN_LIGHT_REVERSE, INPUT);
  pinMode(PIN_COOL_REVERSE, INPUT);*/
  //
  pinMode(PIN_POWER_HUMIDITY, OUTPUT); // 4
  digitalWrite(PIN_POWER_HUMIDITY, LOW);
  pinMode(PIN_COOL, OUTPUT); // 5
  digitalWrite(PIN_COOL, LOW);
  statusCool = STATUS_OFF;
  pinMode(PIN_LIGHT, OUTPUT); // 6
  digitalWrite(PIN_LIGHT, HIGH);
  statusLight = STATUS_OFF;
  pinMode(PIN_PUMP, OUTPUT); // 7
  digitalWrite(PIN_PUMP, HIGH);
  statusPump = STATUS_OFF;
  pinMode(PIN_POWER_WATER_LEVEL, OUTPUT); // 8
  digitalWrite(PIN_POWER_WATER_LEVEL, LOW);
  statusWaterLevel = STATUS_OFF;
  //
  Wire.begin();
  lcd.init();                       //  Инициируем работу с LCD дисплеем
  lcd.backlight();                  //  Включаем подсветку LCD дисплея
  lcd.noAutoscroll();
  lcd.setCursor(0, 0);              //  Устанавливаем курсор в позицию (0 столбец, 0 строка)
  counter = 0;
  alh = alt = aht = 0;
  // init ranges for air rotate
  coolTime[0] = hhmm2min(05, 50); // be careful: 09 is wrong because 0X is octet value
  coolTime[1] = hhmm2min(06, 00);
  coolTime[2] = hhmm2min(11, 00);
  coolTime[3] = hhmm2min(11, 10);
  coolTime[4] = hhmm2min(16, 00);
  coolTime[5] = hhmm2min(16, 10);
  coolTime[6] = hhmm2min(20, 00);
  coolTime[7] = hhmm2min(20, 10);
  clog("read values");
  readTemperature(IS_DUMMY, true);
  humidityLowAlarm();
  tLowAlarm();
  tHighAlarm();
  clog("Init done");
  digitalWrite(PIN_POWER_WATER_LEVEL, HIGH);
}


void loop() {
  u16 now = minutes();
  isDay = TIME_START_DAY <= now && now <= TIME_END_DAY;
  bool temperatureReaded = false;
  if (customChange()) {
    printStatus(now);
    delay(900);
    return;
  }
  counter++;
  if (counter == COUNTER_MAX) {
    counter = 0;
  }
  u16 wl = analogRead(PIN_WATER_LEVEL);
  clog("custom WL: %i", wl);
  // для начала проверим помпу, что она не перелилась, например
  pumpAlarm();

  // потом диапазоны температур, если пришло время
  u8 tempDelayRead = THERMAL_READ_TIME_NORMAL;
  // если счас что-то экстренное, то почаще
  if (statusLight == STATUS_ALARM || statusCool == STATUS_ALARM) {
    tempDelayRead = THERMAL_READ_TIME_ALARMED;
  }
  if ((counter % tempDelayRead) == 0 || needUpdateTemp) {
    readTemperature(IS_DUMMY, false);
    tLowAlarm();
    tHighAlarm();
    temperatureReaded = true;
  }

  // далее проверяем влажность (если пришло время)
  if ((counter % HUMIDITY_READ_TIME) == 0) {
    humidityLowAlarm();
  }
  // если всё ок, то идём в обычную ежедневную программу
  if (!isAlarm()) {
    scheduler(now);
  }

  printStatus(now);

  if (!temperatureReaded) {
    delay(1000);
  }
}

