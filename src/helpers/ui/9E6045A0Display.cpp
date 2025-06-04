#include "9E6045A0Display.h"

bool Display9E6045A0::i2c_probe(TwoWire& wire, uint8_t addr) {
  Serial.printf("9E6045A0: Probing I2C address 0x%02X\n", addr);
  wire.beginTransmission(addr);
  uint8_t error = wire.endTransmission();
  if (error == 0) {
    Serial.println("9E6045A0: I2C probe successful");
  } else {
    Serial.printf("9E6045A0: I2C probe failed with error %d\n", error);
  }
  return (error == 0);
}

bool Display9E6045A0::begin() {
  Serial.println("9E6045A0: Beginning display initialization");

  // First verify the display is present on I2C
  Serial.printf("9E6045A0: Verifying display at address 0x%02X\n", DISPLAY_ADDRESS);
  if (!i2c_probe(Wire, DISPLAY_ADDRESS)) {
    Serial.println("9E6045A0: Display not found on I2C bus!");
    return false;
  }

  // Initialize the SSD1316 display
  Serial.println("9E6045A0: Initializing SSD1316 display");
  if (!display.begin(DISPLAY_ADDRESS, true)) {
    Serial.println("9E6045A0: SSD1316 initialization failed!");
    return false;
  }

  // Configure display settings
  Serial.println("9E6045A0: Configuring display settings");
  display.clearDisplay();
  display.setTextColor(SSD1316_WHITE);
  display.setTextSize(1);
  display.cp437(true);
  
  // Try to write something to the display buffer
  Serial.println("9E6045A0: Testing display by writing text");
  display.setCursor(0,0);
  display.print("TEST");
  display.display();

  _isOn = true;
  Serial.println("9E6045A0: Display initialization complete");
  return true;
}

void Display9E6045A0::turnOn() {
  Serial.println("9E6045A0: Turning display ON");
  if (!_isOn) {
    display.ssd1316_command(0xAF); // Display ON
    _isOn = true;
  }
}

void Display9E6045A0::turnOff() {
  Serial.println("9E6045A0: Turning display OFF");
  if (_isOn) {
    display.ssd1316_command(0xAE); // Display OFF
    _isOn = false;
  }
}

void Display9E6045A0::clear() {
  display.clearDisplay();
  display.display();
}

void Display9E6045A0::startFrame(Color bkg) {
  display.clearDisplay();
  _color = SSD1316_WHITE;
  display.setTextColor(_color);
  display.setTextSize(1);
  display.cp437(true);
}

void Display9E6045A0::setTextSize(int sz) {
  display.setTextSize(sz);
}

void Display9E6045A0::setColor(Color c) {
  _color = (c != 0) ? SSD1316_WHITE : SSD1316_BLACK;
  display.setTextColor(_color);
}

void Display9E6045A0::setCursor(int x, int y) {
  display.setCursor(x, y);
}

void Display9E6045A0::print(const char* str) {
  display.print(str);
}

void Display9E6045A0::fillRect(int x, int y, int w, int h) {
  display.fillRect(x, y, w, h, _color);
}

void Display9E6045A0::drawRect(int x, int y, int w, int h) {
  display.drawRect(x, y, w, h, _color);
}

void Display9E6045A0::drawXbm(int x, int y, const uint8_t* bits, int w, int h) {
  display.drawBitmap(x, y, bits, w, h, SSD1316_WHITE);
}

uint16_t Display9E6045A0::getTextWidth(const char* str) {
  int16_t x1, y1;
  uint16_t w, h;
  display.getTextBounds(str, 0, 0, &x1, &y1, &w, &h);
  return w;
}

void Display9E6045A0::endFrame() {
  display.display();
} 