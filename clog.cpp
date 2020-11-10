

#define BINF "%c%c%c%c%c%c%c%c"

#define B2BINARY(byte)  \
  (byte & 0x80 ? '1' : '0'), \
  (byte & 0x40 ? '1' : '0'), \
  (byte & 0x20 ? '1' : '0'), \
  (byte & 0x10 ? '1' : '0'), \
  (byte & 0x08 ? '1' : '0'), \
  (byte & 0x04 ? '1' : '0'), \
  (byte & 0x02 ? '1' : '0'), \
  (byte & 0x01 ? '1' : '0') 
  
#ifdef DEBUG

void clog(char *format, ...) {
  va_list args;
  char buffer[256];
  va_start(args, format);
  vsnprintf(buffer, 255, format, args);
  Serial.println(buffer);
  va_end(args);
}

void printRTCState() {
  Wire.beginTransmission(DS3231_ADDRESS); // 104 is DS3231 device address
  Wire.write(0x00); // start at register 0
  Wire.endTransmission();
  Wire.requestFrom(DS3231_ADDRESS, 19); // request seven bytes
  byte d;
  float t;
  d = Wire.read();
  clog("param    hex bin");
  clog("seconds: %02x "BINF, d, B2BINARY(d));
  d = Wire.read();
  clog("minutes: %02x "BINF, d, B2BINARY(d));
  d = Wire.read();
  clog("hour:    %02x "BINF, d, B2BINARY(d));
  d = Wire.read();
  clog("day:     %02x "BINF, d, B2BINARY(d));
  d = Wire.read();
  clog("date:    %02x "BINF, d, B2BINARY(d));
  d = Wire.read();
  clog("month:   %02x "BINF, d, B2BINARY(d));
  d = Wire.read();
  clog("year:    %02x "BINF, d, B2BINARY(d));
  d = Wire.read();
  clog("seconds: %02x "BINF, d, B2BINARY(d));
  d = Wire.read();
  clog("minutes: %02x "BINF, d, B2BINARY(d));
  d = Wire.read();
  clog("hour:    %02x "BINF, d, B2BINARY(d));
  d = Wire.read();
  clog("day:     %02x "BINF, d, B2BINARY(d));
  d = Wire.read();
  clog("minutes: %02x "BINF, d, B2BINARY(d));
  d = Wire.read();
  clog("hour:    %02x "BINF, d, B2BINARY(d));
  d = Wire.read();
  clog("day:     %02x "BINF, d, B2BINARY(d));
  d = Wire.read();
  clog("control: %02x "BINF, d, B2BINARY(d));
  d = Wire.read();
  clog("status:  %02x "BINF, d, B2BINARY(d));
  d = Wire.read();
  clog("assign:  %02x "BINF, d, B2BINARY(d));
  t = ((Wire.read() << 2) | (Wire.read() >> 6)) / 4.0;
  char buf[6];
  dtostrf(t, 4, 2, buf);
  clog("temp: %s", buf);
}

#else
#define clog(...)
#define printRTCState(...)
#endif
