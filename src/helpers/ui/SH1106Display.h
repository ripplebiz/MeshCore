#pragma once

#include "DisplayDriver.h"
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SH1106.h>

#ifndef PIN_OLED_RESET
  #define PIN_OLED_RESET        -1 // Reset pin # (or -1 if sharing Arduino reset pin)
#endif

#ifndef DISPLAY_ADDRESS
  #define DISPLAY_ADDRESS   0x3C
#endif

class SH1106Display : public DisplayDriver {
  Adafruit_SH1106 display;
  bool _isOn;
  bool _isDisp;
  uint8_t _color;

  bool i2c_probe(TwoWire& wire, uint8_t addr);
public:
  SH1106Display() : DisplayDriver(132, 64), display(PIN_OLED_RESET) { _isOn = false; }
  bool begin();

  bool isOn() override { return _isOn; }
  void turnOn() override;
  void turnOff() override;
  void clear() override;
  void startFrame(Color bkg = DARK) override;
  void setTextSize(int sz) override;
  void setColor(Color c) override;
  void setCursor(int x, int y) override;
  void print(const char* str) override;
  void fillRect(int x, int y, int w, int h) override;
  void drawRect(int x, int y, int w, int h) override;
  void drawXbm(int x, int y, const uint8_t* bits, int w, int h) override;
  void endFrame() override;
};
