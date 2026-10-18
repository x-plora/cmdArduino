#ifndef CMD_TEST_ARDUINO_H
#define CMD_TEST_ARDUINO_H

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <string>

class __FlashStringHelper;
#define F(value) reinterpret_cast<const __FlashStringHelper *>(value)

class HardwareSerial
{
public:
  std::string input;
  std::string output;
  size_t offset = 0;

  void begin(uint32_t) {}
  int available() const { return offset < input.size(); }
  int read() { return static_cast<unsigned char>(input[offset++]); }
  void print(const char *value) { output += value; }
  void print(const __FlashStringHelper *value) {
    print(reinterpret_cast<const char *>(value));
  }
  void print(char value) { output += value; }
  void print(uint8_t value) { output += std::to_string(value); }
  void println() { output += "\r\n"; }
  template<typename T> void println(T value) { print(value); println(); }
};

extern HardwareSerial Serial;

#endif
