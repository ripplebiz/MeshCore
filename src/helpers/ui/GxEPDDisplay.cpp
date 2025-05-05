
#include "GxEPDDisplay.h"

#ifndef DISPLAY_ROTATION
  #define DISPLAY_ROTATION 3
#endif

#ifdef TECHO_ZOOM
  #define SCALE_X   (1.5625f * 1.5f)   //  200 / 128  (with 1.5 scale)
  #define SCALE_Y   (1.5625f * 1.5f)   //  200 / 128  (with 1.5 scale)
#else
  #define SCALE_X   1.5625f   //  200 / 128
  #define SCALE_Y   1.5625f   //  200 / 128
#endif

bool GxEPDDisplay::begin() {
  display.epd2.selectSPI(SPI1, SPISettings(4000000, MSBFIRST, SPI_MODE0));
  SPI1.begin();
  display.init(115200, true, 2, false);
  display.setRotation(DISPLAY_ROTATION);
  #ifdef TECHO_ZOOM
    display.setFont(&FreeMono9pt7b);
  #endif
  display.setPartialWindow(0, 0, display.width(), display.height());

  display.fillScreen(GxEPD_WHITE);
  display.display(true);
  #if DISP_BACKLIGHT
  pinMode(DISP_BACKLIGHT, OUTPUT);
  #endif
  _init = true;
  return true;
}

void GxEPDDisplay::turnOn() {
  if (!_init) begin();
#if DISP_BACKLIGHT
  digitalWrite(DISP_BACKLIGHT, HIGH);
  _isOn = true;
#endif
}

void GxEPDDisplay::turnOff() {
#if DISP_BACKLIGHT
  digitalWrite(DISP_BACKLIGHT, LOW);
#endif
  _isOn = false;
}

void GxEPDDisplay::clear() {
  display.fillScreen(GxEPD_WHITE);
  display.setTextColor(GxEPD_BLACK);
}

void GxEPDDisplay::startFrame(Color bkg) {
  display.fillScreen(GxEPD_WHITE);
}

void GxEPDDisplay::setTextSize(int sz) {
  display.setTextSize(sz);
}

void GxEPDDisplay::setColor(Color c) {
  display.setTextColor(GxEPD_BLACK);
}

void GxEPDDisplay::setCursor(int x, int y) {
  display.setCursor(x*SCALE_X, (y+10)*SCALE_Y);
}

void GxEPDDisplay::print(const char* str) {
  display.print(str);
}

void GxEPDDisplay::fillRect(int x, int y, int w, int h) {
  display.fillRect(x*SCALE_X, y*SCALE_Y, w*SCALE_X, h*SCALE_Y, GxEPD_BLACK);
}

void GxEPDDisplay::drawRect(int x, int y, int w, int h) {
  display.drawRect(x*SCALE_X, y*SCALE_Y, w*SCALE_X, h*SCALE_Y, GxEPD_BLACK);
}

void GxEPDDisplay::drawXbm(int x, int y, const uint8_t* bits, int w, int h) {
  display.drawBitmap(x*SCALE_X, (y*SCALE_Y) + 10, bits, w, h, GxEPD_BLACK);
}

uint16_t GxEPDDisplay::getTextWidth(const char* str) {
  int16_t x1, y1;
  uint16_t w, h;
  display.getTextBounds(str, 0, 0, &x1, &y1, &w, &h);
  return w / SCALE_X;
}

void GxEPDDisplay::endFrame() {
  display.display(true);
}
