#pragma once

#define RADIOLIB_STATIC_ONLY 1
#include <RadioLib.h>
#include <helpers/RadioLibWrappers.h>
#include <helpers/nrf52/RAK4631Board.h>
#include <helpers/CustomSX1262Wrapper.h>
#include <helpers/AutoDiscoverRTCClock.h>
#include <helpers/SensorManager.h>
#if ENV_INCLUDE_GPS
  #include <helpers/sensors/LocationProvider.h>
  #include <SparkFun_u-blox_GNSS_Arduino_Library.h>
#endif
#ifdef DISPLAY_CLASS
  #include <helpers/ui/SSD1306Display.h>
#endif

#define _BV(x)   (1 << x)

class RAK4631SensorManager: public SensorManager {
  #if ENV_INCLUDE_GPS
    bool gps_active = false;
    bool gps_present = false;
    LocationProvider * _nmea;
    SFE_UBLOX_GNSS ublox_GNSS;
    uint32_t disStandbyPin = 0;
  
    void start_gps();
    void stop_gps();
    void sleep_gps();
    void wake_gps();
    bool gpsIsAwake(uint32_t ioPin);
  #endif

  public:
  #if ENV_INCLUDE_GPS
    RAK4631SensorManager(LocationProvider &nmea): _nmea(&nmea) { }

    bool querySensors(uint8_t requester_permissions, CayenneLPP& telemetry) override;
    void loop() override;
    int getNumSettings() const override;
    const char* getSettingName(int i) const override;
    const char* getSettingValue(int i) const override;
    bool setSettingValue(const char* name, const char* value) override;
  #else
    RAK4631SensorManager() { }
  #endif
    bool begin() override;
};

extern RAK4631Board board;
extern WRAPPER_CLASS radio_driver;
extern AutoDiscoverRTCClock rtc_clock;
extern RAK4631SensorManager sensors;

#ifdef DISPLAY_CLASS
  extern DISPLAY_CLASS display;
#endif

enum {
  POWERMANAGE_ONLINE  = _BV(0),
  DISPLAY_ONLINE      = _BV(1),
  RADIO_ONLINE        = _BV(2),
  GPS_ONLINE          = _BV(3),
  PSRAM_ONLINE        = _BV(4),
  SDCARD_ONLINE       = _BV(5),
  AXDL345_ONLINE      = _BV(6),
  BME280_ONLINE       = _BV(7),
  BMP280_ONLINE       = _BV(8),
  BME680_ONLINE       = _BV(9),
  QMC6310_ONLINE      = _BV(10),
  QMI8658_ONLINE      = _BV(11),
  PCF8563_ONLINE      = _BV(12),
  OSC32768_ONLINE     = _BV(13),
  RAK12500_ONLINE     = _BV(14),
};

bool radio_init();
uint32_t radio_get_rng_seed();
void radio_set_params(float freq, float bw, uint8_t sf, uint8_t cr);
void radio_set_tx_power(uint8_t dbm);
mesh::LocalIdentity radio_new_identity();
