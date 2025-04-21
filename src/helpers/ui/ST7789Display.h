#pragma once

#include "DisplayDriver.h"
#include <Wire.h>
#include <SPI.h>
#include <Adafruit_GFX.h>
#include <Adafruit_ST7789.h>

class ST7789Display : public DisplayDriver {
  Adafruit_ST7789 display;
  bool _isOn;
  uint16_t _color;

  bool i2c_probe(TwoWire& wire, uint8_t addr);
public:
  ST7789Display() : DisplayDriver(135, 240), display(&SPI1, PIN_TFT_CS, PIN_TFT_DC, PIN_TFT_RST) { _isOn = false; }
//  ST7789Display() : DisplayDriver(135, 240), display(PIN_TFT_CS, PIN_TFT_DC, PIN_TFT_SDA, PIN_TFT_SCL, PIN_TFT_RST) { _isOn = false; }
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
