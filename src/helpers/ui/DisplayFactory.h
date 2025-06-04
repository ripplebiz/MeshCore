#pragma once

#include "DisplayDriver.h"
#include "9E6045A0Display.h"
#include "SSD1306Display.h"
#include "SH1106Display.h"
#include "ST7735Display.h"
#include "ST7789Display.h"
#include "GxEPDDisplay.h"
#include <helpers/RefCountedDigitalPin.h>

// Display type enumeration
enum class DisplayType {
    DISPLAY_NONE = 0,
    DISPLAY_SSD1306,
    DISPLAY_SH1106,
    DISPLAY_ST7735,
    DISPLAY_ST7789,
    DISPLAY_GXEPD,
    DISPLAY_9E6045A0
};

class DisplayFactory {
public:
    // Create a display instance based on type
    static DisplayDriver* createDisplay(DisplayType type, RefCountedDigitalPin* peripher_power = nullptr);

    // Try to auto-detect display type by probing I2C and checking pins
    static DisplayType detectDisplayType();

private:
    // Helper function to probe I2C devices
    static bool probeI2CDevice(uint8_t address);
}; 