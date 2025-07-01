#pragma once

#define RADIOLIB_STATIC_ONLY 1
#include <RadioLib.h>
#include <helpers/RadioLibWrappers.h>
#include <helpers/TBeamS3SupremeBoard.h>
#include <helpers/CustomSX1262Wrapper.h>
#include <helpers/AutoDiscoverRTCClock.h>
#include <helpers/SensorManager.h>
#include <helpers/sensors/LocationProvider.h>
#include <Adafruit_BME280.h>

class TbeamSupSensorManager: public SensorManager {
    bool gps_active = false;
    bool bme_active = false;
    LocationProvider * _nmea;
    Adafruit_BME280 bme;
    double node_temp, node_hum, node_pres;
    
    #define SEALEVELPRESSURE_HPA (1013.25)
    
    void start_gps();
    void sleep_gps();
  public:
    TbeamSupSensorManager(LocationProvider &nmea): _nmea(&nmea) {node_temp = 0; node_hum = 0; node_pres = 0;}
    bool begin() override;
    bool querySensors(uint8_t requester_permissions, CayenneLPP& telemetry) override;
    void loop() override;
    int getNumSettings() const override;
    const char* getSettingName(int i) const override;
    const char* getSettingValue(int i) const override;
    bool setSettingValue(const char* name, const char* value) override;

    #ifdef MESH_DEBUG
    void printBMEValues();
    #endif

  };

extern TBeamS3SupremeBoard board;
extern WRAPPER_CLASS radio_driver;
extern AutoDiscoverRTCClock rtc_clock;
extern TbeamSupSensorManager sensors;

#ifdef DISPLAY_CLASS
  #include <helpers/ui/SH1106Display.h>
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
  OSC32768_ONLINE      = _BV(13),
};

bool radio_init();
uint32_t radio_get_rng_seed();
void radio_set_params(float freq, float bw, uint8_t sf, uint8_t cr);
void radio_set_tx_power(uint8_t dbm);
mesh::LocalIdentity radio_new_identity();