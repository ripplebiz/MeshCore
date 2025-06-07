#pragma once

#include "DisplayDriver.h"
#include <Wire.h>
#include <Adafruit_GFX.h>
#define SSD1316_NO_SPLASH
#include <Adafruit_SSD1316.h>

#ifndef PIN_OLED_RESET
#define PIN_OLED_RESET -1  // Reset pin # (or -1 if sharing Arduino reset pin)
#endif

#ifndef DISPLAY_ADDRESS
#define DISPLAY_ADDRESS 0x3C  // Default I2C address
#endif

// Display dimensions
#define SCREEN_WIDTH 96
#define SCREEN_HEIGHT 32

class Display9E6045A0 : public DisplayDriver {
  Adafruit_SSD1316 display;
  bool _isOn;
  uint8_t _color;

  bool i2c_probe(TwoWire& wire, uint8_t addr);

public:
  Display9E6045A0() : DisplayDriver(SCREEN_WIDTH, SCREEN_HEIGHT), 
                      display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, PIN_OLED_RESET) {
    _isOn = false;
  }
  
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
  uint16_t getTextWidth(const char* str) override;
  void endFrame() override;
}; 