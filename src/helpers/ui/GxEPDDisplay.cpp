
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
  // Calculate the base position in display coordinates
  uint16_t startX = x * SCALE_X;
  uint16_t startY = y * SCALE_Y;
  
  // Width in bytes for bitmap processing
  uint16_t widthInBytes = (w + 7) / 8;
  
  // Process the bitmap row by row
  for (uint16_t by = 0; by < h; by++) {
    // Calculate the target y-coordinates for this logical row
    int y1 = startY + (int)(by * SCALE_Y);
    int y2 = startY + (int)((by + 1) * SCALE_Y);
    int block_h = y2 - y1;
    
    // Scan across the row bit by bit
    for (uint16_t bx = 0; bx < w; bx++) {
      // Calculate the target x-coordinates for this logical column
      int x1 = startX + (int)(bx * SCALE_X);
      int x2 = startX + (int)((bx + 1) * SCALE_X);
      int block_w = x2 - x1;
      
      // Get the current bit
      uint16_t byteOffset = (by * widthInBytes) + (bx / 8);
      uint8_t bitMask = 0x80 >> (bx & 7);
      bool bitSet = pgm_read_byte(bits + byteOffset) & bitMask;
      
      // If the bit is set, draw a block of pixels
      if (bitSet) {
        // Draw the block as a filled rectangle
        display.fillRect(x1, y1, block_w, block_h, GxEPD_BLACK);
      }
    }
  }
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
