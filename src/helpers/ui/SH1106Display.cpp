#include "SH1106Display.h"

bool SH1106Display::i2c_probe(TwoWire& wire, uint8_t addr) {
  wire.beginTransmission(addr);
  uint8_t error = wire.endTransmission();
  return (error == 0);
}

bool SH1106Display::begin() {
  _isDisp = i2c_probe(Wire, DISPLAY_ADDRESS);
  if(_isDisp){
    display.begin(SH1106_SWITCHCAPVCC, DISPLAY_ADDRESS, false);
  }
  return _isDisp;
}

void SH1106Display::turnOn() {
  display.SH1106_command(SH1106_DISPLAYON);
  _isOn = true;
}

void SH1106Display::turnOff() {
  display.SH1106_command(SH1106_DISPLAYOFF);
  _isOn = false;
}

void SH1106Display::clear() {
  display.clearDisplay();
  display.display();
}

void SH1106Display::startFrame(Color bkg) {
  display.clearDisplay();  // TODO: apply 'bkg'
  _color = LIGHT;
  display.setTextColor(_color);
  display.setTextSize(1);
  display.cp437(true);         // Use full 256 char 'Code Page 437' font
}

void SH1106Display::setTextSize(int sz) {
  display.setTextSize(sz);
}

void SH1106Display::setColor(Color c) {
  _color = (c != 0) ? LIGHT : DARK;
  display.setTextColor(_color);
}

void SH1106Display::setCursor(int x, int y) {
  display.setCursor(x, y);
}

void SH1106Display::print(const char* str) {
  display.print(str);
}

void SH1106Display::fillRect(int x, int y, int w, int h) {
  display.fillRect(x, y, w, h, _color);
}

void SH1106Display::drawRect(int x, int y, int w, int h) {
  display.drawRect(x, y, w, h, _color);
}

void SH1106Display::drawXbm(int x, int y, const uint8_t* bits, int w, int h) {
  display.drawBitmap(x, y, bits, w, h, LIGHT);
}

void SH1106Display::endFrame() {
  display.display();
}
