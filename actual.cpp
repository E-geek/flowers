
inline u8 hexTimeToDec(u8 time) {
  return (time >> 4) * 10 + (time & 0b1111); // NOLINT(hicpp-signed-bitwise)
}

struct Time {
  u8 h;
  u8 m;
  u8 s;
};

Time actualClockTime() {
  u8 *tmp = read_i2c_register(0x00, 3);
  Time cTime{};
  cTime.s = hexTimeToDec(tmp[0]);
  cTime.m = hexTimeToDec(tmp[1]);
  cTime.h = hexTimeToDec(tmp[2]);
  return cTime;
}
