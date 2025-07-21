#pragma once

#include <Mesh.h>
#include <helpers/SensorManager.h>
#include <helpers/sensors/LocationProvider.h>

#ifdef ENV_INCLUDE_BME680
#include <bsec2.h>
#endif

class EnvironmentSensorManager : public SensorManager {
protected:
  int next_available_channel = TELEM_CHANNEL_SELF + 1;

  bool AHTX0_initialized = false;
  bool BME280_initialized = false;
  bool BMP280_initialized = false;
  bool BME680_initialized = false;
  bool INA3221_initialized = false;
  bool INA219_initialized = false;
  bool SHTC3_initialized = false;
  bool LPS22HB_initialized = false;
  bool MLX90614_initialized = false;
  bool VL53L0X_initialized = false;

  bool gps_detected = false;
  bool gps_active = false;

  #if ENV_INCLUDE_GPS
  LocationProvider* _location;
  void start_gps();
  void stop_gps();
  void initBasicGPS();
  #ifdef RAK_BOARD
  void rakGPSInit();
  bool gpsIsAwake(uint8_t ioPin);
  #endif
  #endif

  #if ENV_INCLUDE_BME680
  static void checkBMEStatus(Bsec2 bsec);
  static void newDataCallback(const bme68xData data, const bsecOutputs outputs, Bsec2 bsec);
  #endif

public:
  #if ENV_INCLUDE_GPS
  EnvironmentSensorManager(LocationProvider &location): _location(&location){};
  #else
  EnvironmentSensorManager(){};
  #endif
  bool begin() override;
  bool querySensors(uint8_t requester_permissions, CayenneLPP& telemetry) override;
  #if ENV_INCLUDE_GPS || ENV_INCLUDE_BME680
  void loop() override;
  #endif
  int getNumSettings() const override;
  const char* getSettingName(int i) const override;
  const char* getSettingValue(int i) const override;
  bool setSettingValue(const char* name, const char* value) override;
};
