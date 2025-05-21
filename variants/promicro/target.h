#pragma once

#define RADIOLIB_STATIC_ONLY 1
#include <RadioLib.h>
#include <helpers/RadioLibWrappers.h>
#include <helpers/nrf52/PromicroBoard.h>
#include <helpers/CustomSX1262Wrapper.h>
#include <helpers/CustomLLCC68Wrapper.h>
#include <helpers/AutoDiscoverRTCClock.h>
#include <helpers/SensorManager.h>
#include <INA3221.h>
#include <INA219.h>
#include <Adafruit_AHTX0.h>
#ifdef DISPLAY_CLASS
  #include <helpers/ui/SSD1306Display.h>
#endif
#include <helpers/sensors/LocationProvider.h>

#define NUM_SENSOR_SETTINGS 3

extern PromicroBoard board;
extern WRAPPER_CLASS radio_driver;
extern AutoDiscoverRTCClock rtc_clock;

#ifdef DISPLAY_CLASS
  extern DISPLAY_CLASS display;
#endif

bool radio_init();
uint32_t radio_get_rng_seed();
void radio_set_params(float freq, float bw, uint8_t sf, uint8_t cr);
void radio_set_tx_power(uint8_t dbm);
mesh::LocalIdentity radio_new_identity();

#define TELEM_INA3221_ADDRESS 0x42      // INA3221 3 channel current sensor I2C address
#define TELEM_INA219_ADDRESS  0x40      // INA219 single channel current sensor I2C address
#define TELEM_AHTX_ADDRESS    0x38      // AHT10, AHT20 temperature and humidity sensor I2C address

#define TELEM_INA3221_SHUNT_VALUE 0.100 // most variants will have a 0.1 ohm shunts
#define TELEM_INA3221_SETTING_CH1 "INA3221-1"
#define TELEM_INA3221_SETTING_CH2 "INA3221-2"
#define TELEM_INA3221_SETTING_CH3 "INA3221-3"

#define TELEM_INA219_SHUNT_VALUE 0.100  // shunt value in ohms (may differ between manufacturers)
#define TELEM_INA219_MAX_CURRENT 5

class PromicroSensorManager: public SensorManager {
  bool gps_active = false;
  bool gps_detected = false;
  LocationProvider* _location;

  bool INA3221initialized = false;
  bool INA219initialized = false;
  bool AHTXinitialized = false;

  // INA3221 channels in telemetry
  const char * INA3221_CHANNEL_NAMES[NUM_SENSOR_SETTINGS] = { TELEM_INA3221_SETTING_CH1, TELEM_INA3221_SETTING_CH2, TELEM_INA3221_SETTING_CH3};
  bool INA3221_CHANNEL_ENABLED[NUM_SENSOR_SETTINGS] = {true, true, true};
  
  void initINA3221();
  void initINA219();
  void initAHTX();

  void initGPS();
  void start_gps();
  void stop_gps();
public:
  PromicroSensorManager(LocationProvider &location): _location(&location) {};
  bool begin() override;
  bool querySensors(uint8_t requester_permissions, CayenneLPP& telemetry) override;
  int getNumSettings() const override;
  const char* getSettingName(int i) const override;
  const char* getSettingValue(int i) const override;
  bool setSettingValue(const char* name, const char* value) override;
  void loop() override;
};


extern PromicroSensorManager sensors;