
uint8_t *read_i2c_register(uint8_t addr, uint8_t reg, byte len) {
  Wire.beginTransmission(addr);
  Wire.write((byte)reg);
  Wire.endTransmission();
  Wire.requestFrom(addr, len);
  uint8_t out[len];
  for(uint8_t i = 0; i < len; i++) {
    out[i] = Wire.read();
  }
  return out;
}

uint8_t *read_i2c_register(uint8_t reg, byte len) {
  return read_i2c_register(DS3231_ADDRESS, reg, len);
}

void write_i2c_register(uint8_t addr, uint8_t reg, uint8_t val) {
  Wire.beginTransmission(addr);
  Wire.write((byte)reg);
  Wire.write((byte)val);
  Wire.endTransmission();
}

void write_i2c_register(uint8_t reg, uint8_t val) {
  return write_i2c_register(DS3231_ADDRESS, reg, val);
}