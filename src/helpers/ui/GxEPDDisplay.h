#pragma once

#include <SPI.h>
#include <Wire.h>

#define ENABLE_GxEPD2_GFX 0

#include <GxEPD2_BW.h>
#include <GxEPD2_3C.h>
#include <GxEPD2_4C.h>
#include <GxEPD2_7C.h>
#include <Fonts/FreeSans9pt7b.h>
#include <Fonts/FreeSansBold12pt7b.h>
#include <Fonts/FreeSans18pt7b.h>

#define GxEPD2_DISPLAY_CLASS GxEPD2_BW
#define GxEPD2_DRIVER_CLASS GxEPD2_150_BN  // DEPG0150BN 200x200, SSD1681, (FPC8101), TTGO T5 V2.4.1

#include <epd/GxEPD2_150_BN.h>  // 1.54" b/w

#include "DisplayDriver.h"

//GxEPD2_BW<GxEPD2_150_BN, 200> display(GxEPD2_150_BN(DISP_CS, DISP_DC, DISP_RST, DISP_BUSY)); // DEPG0150BN 200x200, SSD1681, TTGO T5 V2.4.1


class GxEPDDisplay : public DisplayDriver {

  GxEPD2_BW<GxEPD2_150_BN, 200> display;
  bool _init = false;
  bool _isOn = false;
  uint16_t _curr_color;

public:
  // there is a margin in y...
  GxEPDDisplay() : DisplayDriver(128, 128), display(GxEPD2_150_BN(DISP_CS, DISP_DC, DISP_RST, DISP_BUSY)) {
  }

  bool begin();

  bool isOn() override {return _isOn;};
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
