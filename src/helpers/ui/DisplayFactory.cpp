#include "DisplayFactory.h"
#include <Wire.h>
#include "XPowersLib.h"

// PMU instance for power management
static XPowersAXP2101 PMU;

DisplayDriver* DisplayFactory::createDisplay(DisplayType type, RefCountedDigitalPin* peripher_power) {
    Serial.println("DisplayFactory: Starting display initialization");
    
    // Initialize PMU first using Wire1
    Serial.println("DisplayFactory: Initializing PMU...");
    bool pmu_ok = PMU.begin(Wire1, AXP2101_SLAVE_ADDRESS, PIN_BOARD_SDA1, PIN_BOARD_SCL1);
    if (!pmu_ok) {
        Serial.println("DisplayFactory: Failed to initialize PMU!");
        return nullptr;
    }

    // Power up the display using ALDO1
    Serial.println("DisplayFactory: Setting up display power...");
    PMU.setALDO1Voltage(3300);  // 3.3V for OLED
    PMU.enableALDO1();
    delay(50);  // Give power time to stabilize

    // Initialize I2C for display
    Serial.printf("DisplayFactory: Setting up display I2C on SDA=%d, SCL=%d\n", PIN_BOARD_SDA, PIN_BOARD_SCL);
    Wire.begin(PIN_BOARD_SDA, PIN_BOARD_SCL);
    Wire.setClock(400000); // Use 400kHz I2C

    // Scan I2C bus for devices
    Serial.println("DisplayFactory: Scanning I2C bus...");
    for (uint8_t addr = 0x01; addr < 0x7F; addr++) {
        Wire.beginTransmission(addr);
        uint8_t error = Wire.endTransmission();
        if (error == 0) {
            Serial.printf("DisplayFactory: Found I2C device at address 0x%02X\n", addr);
        }
    }

    // Create and return the appropriate display instance based on type
    Serial.printf("DisplayFactory: Creating display type %d\n", (int)type);
    switch (type) {
        case DisplayType::DISPLAY_9E6045A0: {
            Serial.println("DisplayFactory: Initializing 9E6045A0 display...");
            auto display = new Display9E6045A0();
            if (!display->begin()) {
                Serial.println("DisplayFactory: Failed to initialize 9E6045A0 display!");
                delete display;
                return nullptr;
            }
            Serial.println("DisplayFactory: 9E6045A0 display initialized successfully");
            return display;
        }
        case DisplayType::DISPLAY_SSD1306:
            return new SSD1306Display();
        case DisplayType::DISPLAY_SH1106:
            return new SH1106Display();
        case DisplayType::DISPLAY_ST7735:
            return new ST7735Display();
        case DisplayType::DISPLAY_ST7789:
            return new ST7789Display();
        case DisplayType::DISPLAY_GXEPD:
            return new GxEPDDisplay();
        default:
            Serial.println("DisplayFactory: Unknown display type!");
            return nullptr;
    }
}

DisplayType DisplayFactory::detectDisplayType() {
    Serial.println("DisplayFactory: Detecting display type...");
    
    // Initialize PMU first using Wire1
    Serial.println("DisplayFactory: Initializing PMU...");
    bool pmu_ok = PMU.begin(Wire1, AXP2101_SLAVE_ADDRESS, PIN_BOARD_SDA1, PIN_BOARD_SCL1);
    if (!pmu_ok) {
        Serial.println("DisplayFactory: Failed to initialize PMU!");
        return DisplayType::DISPLAY_NONE;
    }

    // Power up the display using ALDO1
    Serial.println("DisplayFactory: Setting up display power...");
    PMU.setALDO1Voltage(3300);  // 3.3V for OLED
    PMU.enableALDO1();
    delay(50);  // Give power time to stabilize
    
    // Initialize I2C if not already done
    Serial.printf("DisplayFactory: Setting up I2C on SDA=%d, SCL=%d\n", PIN_BOARD_SDA, PIN_BOARD_SCL);
    Wire.begin(PIN_BOARD_SDA, PIN_BOARD_SCL);
    Wire.setClock(400000); // Use 400kHz I2C
    
    // Known I2C addresses for different displays
    const uint8_t SSD1306_ADDR = 0x3C;  // Also used by 9E6045A0
    const uint8_t SH1106_ADDR = 0x3C;   // Same address as SSD1306
    
    // First check I2C displays since they're easier to detect
    Serial.printf("DisplayFactory: Probing for display at address 0x%02X...\n", SSD1306_ADDR);
    if (probeI2CDevice(SSD1306_ADDR)) {
        Serial.println("DisplayFactory: Found display at 0x3C, assuming 9E6045A0");
        // Both SSD1306 and 9E6045A0 use the same address
        // The 9E6045A0 is specifically for T-Beam Supreme with 96x32 resolution
        // We can assume it's a 9E6045A0 in this case since we're on a T-Beam Supreme
        return DisplayType::DISPLAY_9E6045A0;
    }

    Serial.println("DisplayFactory: No I2C display found");
    return DisplayType::DISPLAY_NONE;
}

bool DisplayFactory::probeI2CDevice(uint8_t address) {
    Wire.beginTransmission(address);
    uint8_t error = Wire.endTransmission();
    if (error == 0) {
        Serial.printf("DisplayFactory: Successfully probed device at 0x%02X\n", address);
    } else {
        Serial.printf("DisplayFactory: Failed to probe device at 0x%02X (error=%d)\n", address, error);
    }
    return (error == 0);
} 