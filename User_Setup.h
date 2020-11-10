#ifndef _UNTITLED_USER_SETUP_H_
#define _UNTITLED_USER_SETUP_H_

#define IS_DUMMY false

#define LCD_ON true

#define TIME_START_DAY 360
#define TIME_END_DAY   1320

#define COUNTER_MAX 54000

#define STATUS_OFF    0
#define STATUS_ON     1
#define STATUS_ALARM  2

#define DS3231_ADDRESS  0x68

#define PIN_THERMAL 3

#define TEMP_LOW 20 // ниже нельзы
#define TEMP_MIN 22 // приемлимый минимум
#define TEMP_MAX 33 // приемлимый максимум
#define TEMP_HIGH 36 // выше нельзя

// частота опроса температуры в обычном режиме 5мин
#define THERMAL_READ_TIME_NORMAL 100
// частота опроса температуры в критическом режиме по температуре 5сек
#define THERMAL_READ_TIME_ALARMED 5

// порт датчика воды
// с ним есть странность: он срабатывает на 400, но резко взмывает до 600 не пройдя и 1см
// в центре значене около 650, а максимум -- 710, но это вода с небольшими солями
#define PIN_WATER_LEVEL 3
// уровень сигнала, когда пора остановиться набирть воду
#define WATER_FULL 400

// порты датчиков влажности на аналоге
#define PIN_HUMIDITY_0 0
#define PIN_HUMIDITY_1 1
#define PIN_HUMIDITY_2 2

// влажность ниже которой нужно производить полив
#define HUMIDITY_MIN 380
// влажность дежурно минимальная
#define HUMIDITY_CRIT 250
// влажность ниже которой значит датчик отключен
#define HUMIDITY_DISABLE 50
// т.к. данные собираются с трёх датчиков, то берутся минимум и максимум

// это верхний показатель аналога, из него вычитаем данные, чтобы получить прямую шкалу
#define HUMIDITY_TOP 1023

// частота опроса влажности 15мин
#define HUMIDITY_READ_TIME 900

// питание контроллера влаги
#define PIN_POWER_HUMIDITY 4
// питание контроллера вентилятора
#define PIN_COOL 5
// питание света
#define PIN_LIGHT 6
// питание помпы
#define PIN_PUMP 7
// питание контроллера уровня воды
#define PIN_POWER_WATER_LEVEL 8

#define PIN_PUMP_REVERSE 9

#define PIN_LIGHT_REVERSE 10

#define PIN_COOL_REVERSE 11


typedef uint8_t u8;
typedef uint16_t u16;

#endif //_UNTITLED_USER_SETUP_H_
