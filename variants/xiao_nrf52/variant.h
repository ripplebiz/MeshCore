#pragma once

#include "WVariant.h"

/** Master clock frequency */
#define VARIANT_MCK       (64000000ul)

#define USE_LFXO      // Board uses 32khz crystal for LF
//#define USE_LFRC    // Board uses RC for LF

#define PINS_COUNT              (33)
#define NUM_DIGITAL_PINS        (33)
#define NUM_ANALOG_INPUTS       (8)
#define NUM_ANALOG_OUTPUTS      (0)

// LEDs
#define PIN_LED                 (LED_RED)
#define LED_PWR                 (PINS_COUNT)
#define PIN_NEOPIXEL            (PINS_COUNT)
#define NEOPIXEL_NUM            (0)

#define LED_BUILTIN             (PIN_LED)

#define LED_RED                 (11)
#define LED_GREEN               (13)
#define LED_BLUE                (12)

#define LED_STATE_ON            (1)     // State when LED is litted

// Buttons
#define PIN_BUTTON1             (PINS_COUNT)

#define VBAT_ENABLE             (14)    // Output LOW to enable reading of the BAT voltage.
                                        // https://wiki.seeedstudio.com/XIAO_BLE#q3-what-are-the-considerations-when-using-xiao-nrf52840-sense-for-battery-charging

#define PIN_CHARGING_CURRENT    (22)    // Battery Charging current
                                        // https://wiki.seeedstudio.com/XIAO_BLE#battery-charging-current

// Analog pins
#define PIN_VBAT                (32)    // Read the BAT voltage.
                                        // https://wiki.seeedstudio.com/XIAO_BLE#q3-what-are-the-considerations-when-using-xiao-nrf52840-sense-for-battery-charging

#define BAT_NOT_CHARGING        (23)    // LOW when charging

#define AREF_VOLTAGE            (3.0)
#define ADC_MULTIPLIER          (3.0F) // 1M, 512k divider bridge

#define ADC_RESOLUTION          (12)

// Other pins
#define PIN_NFC1                (30)
#define PIN_NFC2                (31)

// Serial interfaces
#define PIN_SERIAL1_RX          (7)
#define PIN_SERIAL1_TX          (6)

// SPI Interfaces
#define SPI_INTERFACES_COUNT    (2)

#define PIN_SPI_MISO            (9)
#define PIN_SPI_MOSI            (10)
#define PIN_SPI_SCK             (8)

#define PIN_SPI1_MISO           (25)
#define PIN_SPI1_MOSI           (26)
#define PIN_SPI1_SCK            (29)

// Wire Interfaces
#define WIRE_INTERFACES_COUNT   (1)

#define PIN_WIRE_SDA            (17) // 4 and 5 are used for the sx1262 !
#define PIN_WIRE_SCL            (16) // use WIRE1_SDA

#define PIN_LSM6DS3TR_C_POWER   (15)
#define PIN_LSM6DS3TR_C_INT1    (18)

// PDM Interfaces
#define PIN_PDM_PWR	            (19)
#define PIN_PDM_CLK	            (20)
#define PIN_PDM_DIN	            (21)

// QSPI Pins
#define PIN_QSPI_SCK            (24)
#define PIN_QSPI_CS             (25)
#define PIN_QSPI_IO0            (26)
#define PIN_QSPI_IO1            (27)
#define PIN_QSPI_IO2            (28)
#define PIN_QSPI_IO3            (29)

// On-board QSPI Flash
#define EXTERNAL_FLASH_DEVICES  (P25Q16H)
#define EXTERNAL_FLASH_USE_QSPI
