#pragma once

#define RADIOLIB_STATIC_ONLY 1
#include <RadioLib.h>
#include <helpers/RadioLibWrappers.h>
#include <helpers/stm32/STM32Board.h>
#include <helpers/CustomSTM32WLxWrapper.h>
#include <helpers/ArduinoHelpers.h>
#include <helpers/SensorManager.h>

#include <BME280I2C.h>
#include <Wire.h>

class WIOE5Board : public STM32Board {
public:
    const char* getManufacturerName() const override {
        return "Seeed Wio E5 mini";
    }

    uint16_t getBattMilliVolts() override {
        analogReadResolution(12);
        uint32_t raw = analogRead(PIN_A3);            
        return raw;
    }
};

class WIOE5SensorManager : public SensorManager {
    BME280I2C bme;
    bool has_bme = false;   

public:
    WIOE5SensorManager() {}
    bool begin() override;
    bool querySensors(uint8_t requester_permissions, CayenneLPP& telemetry) override;
};

extern WIOE5Board board;
extern WRAPPER_CLASS radio_driver;
extern VolatileRTCClock rtc_clock;
extern WIOE5SensorManager sensors;

bool radio_init();
uint32_t radio_get_rng_seed();
void radio_set_params(float freq, float bw, uint8_t sf, uint8_t cr);
void radio_set_tx_power(uint8_t dbm);
mesh::LocalIdentity radio_new_identity();
