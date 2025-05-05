#ifdef ST7789

#include "ST7789Display.h"

#ifndef X_OFFSET
#define X_OFFSET 0  // No offset needed for landscape
#endif

#ifndef Y_OFFSET
#define Y_OFFSET 1  // Vertical offset to prevent top row cutoff
#endif

#define SCALE_X  1.875f     // 240 / 128
#define SCALE_Y  2.109375f   // 135 / 64

bool ST7789Display::begin() {
  if(!_isOn) {
    pinMode(PIN_TFT_VDD_CTL, OUTPUT);
    pinMode(PIN_TFT_LEDA_CTL, OUTPUT);
    digitalWrite(PIN_TFT_VDD_CTL, LOW);
    digitalWrite(PIN_TFT_LEDA_CTL, LOW);
    digitalWrite(PIN_TFT_RST, HIGH);

    display.init();
    display.landscapeScreen();
    display.displayOn();
    setCursor(0,0);

    _isOn = true;
  }
  return true;
}

void ST7789Display::turnOn() {
  ST7789Display::begin();
}

void ST7789Display::turnOff() {
  digitalWrite(PIN_TFT_VDD_CTL, HIGH);
  digitalWrite(PIN_TFT_LEDA_CTL, HIGH);
  digitalWrite(PIN_TFT_RST, LOW);
  _isOn = false;
}

void ST7789Display::clear() {
  display.clear();
}

void ST7789Display::startFrame(Color bkg) {
  display.clear();
}

void ST7789Display::setTextSize(int sz) {
  switch(sz) {
    case 1 :
      display.setFont(ArialMT_Plain_16);
      break;
    case 2 :
      display.setFont(ArialMT_Plain_24);
      break;
    default:
      display.setFont(ArialMT_Plain_16);
  }
}

void ST7789Display::setColor(Color c) {
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
  display.setRGB(_color);
}

void ST7789Display::setCursor(int x, int y) {
  _x = x*SCALE_X + X_OFFSET;
  _y = y*SCALE_Y + Y_OFFSET;
}

void ST7789Display::print(const char* str) {
  display.drawString(_x, _y, str);
}

void ST7789Display::fillRect(int x, int y, int w, int h) {
  display.fillRect(x*SCALE_X + X_OFFSET, y*SCALE_Y + Y_OFFSET, w*SCALE_X, h*SCALE_Y);
}

void ST7789Display::drawRect(int x, int y, int w, int h) {
  display.drawRect(x*SCALE_X + X_OFFSET, y*SCALE_Y + Y_OFFSET, w*SCALE_X, h*SCALE_Y);
}

void ST7789Display::drawXbm(int x, int y, const uint8_t* bits, int w, int h) {
  display.drawBitmap(x*SCALE_X + X_OFFSET, y*SCALE_Y + Y_OFFSET, w, h, bits);
}

uint16_t ST7789Display::getTextWidth(const char* str) {
  return display.getStringWidth(str) / SCALE_X;
}

void ST7789Display::endFrame() {
  display.display();
}

#endif