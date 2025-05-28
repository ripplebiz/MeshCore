#pragma once

#include <Arduino.h>

class RefCountedDigitalPin {
  uint8_t _pin;
  int8_t _claims = 0;

public:
  RefCountedDigitalPin(uint8_t pin): _pin(pin) { }

  void begin() {
    pinMode(_pin, OUTPUT);
    digitalWrite(_pin, LOW);  // initial state
  }

  void claim() {
    _claims++;
    if (_claims > 0) {
      digitalWrite(_pin, HIGH);
    }
  }
  void release() {
    _claims--;
    if (_claims == 0) {
      digitalWrite(_pin, LOW);
    }
  }
};
