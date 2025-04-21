
#include "GxEPDDisplay.h"

bool GxEPDDisplay::begin() {
  display.epd2.selectSPI(SPI1, SPISettings(4000000, MSBFIRST, SPI_MODE0));
  SPI1.begin();
  display.init(115200, true, 2, false);
  display.setRotation(3);
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
#ifdef TECHO_ZOOM
  x = x + (x >> 1);
  y = y + (y >> 1);
#endif
  display.setCursor(x, (y+10));
}

void GxEPDDisplay::print(const char* str) {
  display.print(str);
}

void GxEPDDisplay::fillRect(int x, int y, int w, int h) {
#ifdef TECHO_ZOOM
  x = x + (x >> 1);
  y = y + (y >> 1);
  w = w + (w >> 1);
  h = h + (h >> 1);
#endif
  display.fillRect(x, y, w, h, GxEPD_BLACK);
}

void GxEPDDisplay::drawRect(int x, int y, int w, int h) {
#ifdef TECHO_ZOOM
  x = x + (x >> 1);
  y = y + (y >> 1);
  w = w + (w >> 1);
  h = h + (h >> 1);
#endif
  display.drawRect(x, y, w, h, GxEPD_BLACK);
}

void GxEPDDisplay::drawXbm(int x, int y, const uint8_t* bits, int w, int h) {
#ifdef TECHO_ZOOM
  x = x + (x >> 1);
  y = y + (y >> 1);
  w = w + (w >> 1);
  h = h + (h >> 1);
#endif
  display.drawBitmap(x*1.5, (y*1.5) + 10, bits, w, h, GxEPD_BLACK);
}

void GxEPDDisplay::endFrame() {
  display.display(true);
}
