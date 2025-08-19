#include "ST7735Display.h"

#ifndef DISPLAY_ROTATION
  #define DISPLAY_ROTATION 2
#endif

#ifndef DISPLAY_SCALE_X
  #define DISPLAY_SCALE_X  1.25f
#endif
#ifndef DISPLAY_SCALE_Y
  #define DISPLAY_SCALE_Y  1.25f
#endif
#define SCALE_X DISPLAY_SCALE_X
#define SCALE_Y DISPLAY_SCALE_Y

#ifndef TFT_OFFSET_X
  #define TFT_OFFSET_X 0
#endif
#ifndef TFT_OFFSET_Y
  #define TFT_OFFSET_Y 0
#endif

bool ST7735Display::i2c_probe(TwoWire& wire, uint8_t addr) {
  return true;
/*
  wire.beginTransmission(addr);
  uint8_t error = wire.endTransmission();
  return (error == 0);
*/
}

bool ST7735Display::begin() {
  if (!_isOn) {
    if (_peripher_power) _peripher_power->claim();

    pinMode(PIN_TFT_LEDA_CTL, OUTPUT);
    digitalWrite(PIN_TFT_LEDA_CTL, HIGH);
#if defined(PIN_TFT_RST) && (PIN_TFT_RST >= 0)
    digitalWrite(PIN_TFT_RST, HIGH);
#endif

  #ifdef DISPLAY_INITR
    display.initR(DISPLAY_INITR);
  #else
    display.initR(INITR_MINI160x80_PLUGIN);
  #endif
    display.setRotation(DISPLAY_ROTATION);
    display.setSPISpeed(40000000);
    display.fillScreen(ST77XX_BLACK);
    display.setTextColor(ST77XX_WHITE);
    display.setTextSize(1);
    display.cp437(true);         // Use full 256 char 'Code Page 437' font
  
  #if defined(DISPLAY_CENTER_CONTENT)
    #ifndef DISPLAY_PANEL_W
      #define DISPLAY_PANEL_W 160
    #endif
    #ifndef DISPLAY_PANEL_H
      #define DISPLAY_PANEL_H 80
    #endif
    const float physicalW = (float)DISPLAY_PANEL_W;
    const float physicalH = (float)DISPLAY_PANEL_H;
    const float logicalW = 128.0f * SCALE_X;
    const float logicalH = 64.0f * SCALE_Y;
    float offX = (physicalW - logicalW) * 0.5f;
    float offY = (physicalH - logicalH) * 0.5f;
    _offsetX = (int16_t)(offX + 0.5f) + TFT_OFFSET_X;
    _offsetY = (int16_t)(offY + 0.5f) + TFT_OFFSET_Y;
  #else
    _offsetX = TFT_OFFSET_X;
    _offsetY = TFT_OFFSET_Y;
  #endif
  
    _isOn = true;
  }
  return true;
}

void ST7735Display::turnOn() {
  ST7735Display::begin();
}

void ST7735Display::turnOff() {
  if (_isOn) {
    digitalWrite(PIN_TFT_LEDA_CTL, HIGH);
  #if defined(PIN_TFT_RST) && (PIN_TFT_RST >= 0)
    digitalWrite(PIN_TFT_RST, LOW);
  #endif
    digitalWrite(PIN_TFT_LEDA_CTL, LOW);
    _isOn = false;

    if (_peripher_power) _peripher_power->release();
  }
}

void ST7735Display::clear() {
  //Serial.println("DBG: display.Clear");
  display.fillScreen(ST77XX_BLACK);
}

void ST7735Display::startFrame(Color bkg) {
  display.fillScreen(ST77XX_BLACK);
  display.setTextColor(ST77XX_WHITE);
  display.setTextSize(1);
  display.cp437(true);         // Use full 256 char 'Code Page 437' font
}

void ST7735Display::setTextSize(int sz) {
  display.setTextSize(sz);
}

void ST7735Display::setColor(Color c) {
  switch (c) {
    case DisplayDriver::DARK :
      _color = ST77XX_BLACK;
      break;
    case DisplayDriver::LIGHT : 
      _color = ST77XX_WHITE;
      break;
    case DisplayDriver::RED : 
      _color = ST77XX_RED;
      break;
    case DisplayDriver::GREEN : 
      _color = ST77XX_GREEN;
      break;
    case DisplayDriver::BLUE : 
      _color = ST77XX_BLUE;
      break;
    case DisplayDriver::YELLOW : 
      _color = ST77XX_YELLOW;
      break;
    case DisplayDriver::ORANGE : 
      _color = ST77XX_ORANGE;
      break;
    default:
      _color = ST77XX_WHITE;
      break;
  }
  display.setTextColor(_color);
}

void ST7735Display::setCursor(int x, int y) {
  display.setCursor((x*SCALE_X) + _offsetX, (y*SCALE_Y) + _offsetY);
}

void ST7735Display::print(const char* str) {
  display.print(str);
}

void ST7735Display::fillRect(int x, int y, int w, int h) {
  display.fillRect((x*SCALE_X) + _offsetX, (y*SCALE_Y) + _offsetY, w*SCALE_X, h*SCALE_Y, _color);
}

void ST7735Display::drawRect(int x, int y, int w, int h) {
  display.drawRect((x*SCALE_X) + _offsetX, (y*SCALE_Y) + _offsetY, w*SCALE_X, h*SCALE_Y, _color);
}

void ST7735Display::drawXbm(int x, int y, const uint8_t* bits, int w, int h) {
  display.drawBitmap((x*SCALE_X) + _offsetX, (y*SCALE_Y) + _offsetY, bits, w, h, _color);
}

uint16_t ST7735Display::getTextWidth(const char* str) {
  int16_t x1, y1;
  uint16_t w, h;
  display.getTextBounds(str, 0, 0, &x1, &y1, &w, &h);
  uint16_t logical = (uint16_t)(w / SCALE_X);
  if (logical > width()) logical = width();
  return logical;
}

void ST7735Display::endFrame() {
  // display.display();
}
