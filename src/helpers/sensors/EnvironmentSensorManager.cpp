#include "EnvironmentSensorManager.h"

#if ENV_INCLUDE_AHTX0
#define TELEM_AHTX_ADDRESS      0x38      // AHT10, AHT20 temperature and humidity sensor I2C address
#include <Adafruit_AHTX0.h>
static Adafruit_AHTX0 AHTX0;
#endif

#if ENV_INCLUDE_BME280
#ifndef TELEM_BME280_ADDRESS
#define TELEM_BME280_ADDRESS    0x76      // BME280 environmental sensor I2C address
#endif
#define TELEM_BME280_SEALEVELPRESSURE_HPA (1013.25)    // Athmospheric pressure at sea level
#include <Adafruit_BME280.h>
static Adafruit_BME280 BME280;
#endif

#if ENV_INCLUDE_BMP280
#ifndef TELEM_BMP280_ADDRESS
#define TELEM_BMP280_ADDRESS    0x76      // BMP280 environmental sensor I2C address
#endif
#define TELEM_BMP280_SEALEVELPRESSURE_HPA (1013.25)    // Athmospheric pressure at sea level
#include <Adafruit_BMP280.h>
static Adafruit_BMP280 BMP280;
#endif


#if ENV_INCLUDE_INA3221
#define TELEM_INA3221_ADDRESS   0x42      // INA3221 3 channel current sensor I2C address
#define TELEM_INA3221_SHUNT_VALUE 0.100 // most variants will have a 0.1 ohm shunts
#define TELEM_INA3221_NUM_CHANNELS 3
#include <Adafruit_INA3221.h>
static Adafruit_INA3221 INA3221;
#endif

#if ENV_INCLUDE_INA219
#define TELEM_INA219_ADDRESS    0x40      // INA219 single channel current sensor I2C address
#include <Adafruit_INA219.h>
static Adafruit_INA219 INA219(TELEM_INA219_ADDRESS);
#endif

bool EnvironmentSensorManager::begin() {
  #if ENV_INCLUDE_GPS
  initBasicGPS();
  #endif

  #if ENV_INCLUDE_AHTX0
  if (AHTX0.begin(&Wire, 0, TELEM_AHTX_ADDRESS)) {
    MESH_DEBUG_PRINTLN("Found AHT10/AHT20 at address: %02X", TELEM_AHTX_ADDRESS);
    AHTX0_initialized = true;
  } else {
    AHTX0_initialized = false;
    MESH_DEBUG_PRINTLN("AHT10/AHT20 was not found at I2C address %02X", TELEM_AHTX_ADDRESS);
  }
  #endif

  #if ENV_INCLUDE_BME280
  if (BME280.begin(TELEM_BME280_ADDRESS, &Wire)) {
    MESH_DEBUG_PRINTLN("Found BME280 at address: %02X", TELEM_BME280_ADDRESS);
    MESH_DEBUG_PRINTLN("BME sensor ID: %02X", BME280.sensorID());
    BME280_initialized = true;
  } else {
    BME280_initialized = false;
    MESH_DEBUG_PRINTLN("BME280 was not found at I2C address %02X", TELEM_BME280_ADDRESS);
  }
  #endif

  #if ENV_INCLUDE_BMP280
  if (BMP280.begin(TELEM_BMP280_ADDRESS)) {
    MESH_DEBUG_PRINTLN("Found BMP280 at address: %02X", TELEM_BMP280_ADDRESS);
    MESH_DEBUG_PRINTLN("BMP sensor ID: %02X", BMP280.sensorID());
    BMP280_initialized = true;
  } else {
    BMP280_initialized = false;
    MESH_DEBUG_PRINTLN("BMP280 was not found at I2C address %02X", TELEM_BMP280_ADDRESS);
  }
  #endif

  #if ENV_INCLUDE_INA3221
  if (INA3221.begin(TELEM_INA3221_ADDRESS, &Wire)) {
      MESH_DEBUG_PRINTLN("Found INA3221 at address: %02X", TELEM_INA3221_ADDRESS);
      MESH_DEBUG_PRINTLN("%04X %04X", INA3221.getDieID(), INA3221.getManufacturerID());

      for(int i = 0; i < 3; i++) {
        INA3221.setShuntResistance(i, TELEM_INA3221_SHUNT_VALUE);
      }
      INA3221_initialized = true;
  } else {
      INA3221_initialized = false;
      MESH_DEBUG_PRINTLN("INA3221 was not found at I2C address %02X", TELEM_INA3221_ADDRESS);
  }
  #endif

  #if ENV_INCLUDE_INA219
  if (INA219.begin(&Wire)) {
      MESH_DEBUG_PRINTLN("Found INA219 at address: %02X", TELEM_INA219_ADDRESS);
      INA219_initialized = true;
  } else {
      INA219_initialized = false;
      MESH_DEBUG_PRINTLN("INA219 was not found at I2C address %02X", TELEM_INA219_ADDRESS);
  }
  #endif

  return true;
}

bool EnvironmentSensorManager::querySensors(uint8_t requester_permissions, CayenneLPP& telemetry) {
  next_available_channel = TELEM_CHANNEL_SELF + 1;

  if (requester_permissions & TELEM_PERM_LOCATION && gps_active) {
    telemetry.addGPS(TELEM_CHANNEL_SELF, node_lat, node_lon, 0.0f); // allow lat/lon via telemetry even if no GPS is detected
  }

  if (requester_permissions & TELEM_PERM_ENVIRONMENT) {

    #if ENV_INCLUDE_AHTX0
    if (AHTX0_initialized) {
      sensors_event_t humidity, temp;
      AHTX0.getEvent(&humidity, &temp);
      telemetry.addTemperature(TELEM_CHANNEL_SELF, temp.temperature);
      telemetry.addRelativeHumidity(TELEM_CHANNEL_SELF, humidity.relative_humidity);
    }
    #endif

    #if ENV_INCLUDE_BME280
    if (BME280_initialized) {
      telemetry.addTemperature(TELEM_CHANNEL_SELF, BME280.readTemperature());
      telemetry.addRelativeHumidity(TELEM_CHANNEL_SELF, BME280.readHumidity());
      telemetry.addBarometricPressure(TELEM_CHANNEL_SELF, BME280.readPressure());
      telemetry.addAltitude(TELEM_CHANNEL_SELF, BME280.readAltitude(TELEM_BME280_SEALEVELPRESSURE_HPA));
    }
    #endif

    #if ENV_INCLUDE_BMP280
    if (BMP280_initialized) {
      telemetry.addTemperature(TELEM_CHANNEL_SELF, BMP280.readTemperature());
      telemetry.addBarometricPressure(TELEM_CHANNEL_SELF, BMP280.readPressure());
      telemetry.addAltitude(TELEM_CHANNEL_SELF, BME280.readAltitude(TELEM_BME280_SEALEVELPRESSURE_HPA));
    }
    #endif

    #if ENV_INCLUDE_INA3221
    if (INA3221_initialized) {
      for(int i = 0; i < TELEM_INA3221_NUM_CHANNELS; i++) {
        // add only enabled INA3221 channels to telemetry
        if (INA3221.isChannelEnabled(i)) {
          float voltage = INA3221.getBusVoltage(i);
          float current = INA3221.getCurrentAmps(i);
          telemetry.addVoltage(next_available_channel, voltage);
          telemetry.addCurrent(next_available_channel, current);
          telemetry.addPower(next_available_channel, voltage * current);
          next_available_channel++;
        }
      }
    }
    #endif

    #if ENV_INCLUDE_INA219
    if (INA219_initialized) {
        telemetry.addVoltage(next_available_channel, INA219.getBusVoltage_V());
        telemetry.addCurrent(next_available_channel, INA219.getCurrent_mA() / 1000);
        telemetry.addPower(next_available_channel, INA219.getPower_mW() / 1000);
        next_available_channel++;
    }
    #endif

  }

  return true;
}


int EnvironmentSensorManager::getNumSettings() const {
  #if ENV_INCLUDE_GPS
    return gps_detected ? 1 : 0;  // only show GPS setting if GPS is detected
  #else
    return 0;
  #endif
}

const char* EnvironmentSensorManager::getSettingName(int i) const {
  #if ENV_INCLUDE_GPS
    return (gps_detected && i == 0) ? "gps" : NULL;
  #else
    return NULL;
  #endif
}

const char* EnvironmentSensorManager::getSettingValue(int i) const {
  #if ENV_INCLUDE_GPS
  if (gps_detected && i == 0) {
    return gps_active ? "1" : "0";
  }
  #endif
  return NULL;
}

bool EnvironmentSensorManager::setSettingValue(const char* name, const char* value) {
  #if ENV_INCLUDE_GPS
  if (gps_detected && strcmp(name, "gps") == 0) {
    if (strcmp(value, "0") == 0) {
      stop_gps();
    } else {
      start_gps();
    }
    return true;
  }
  #endif
  return false;  // not supported
}

#if ENV_INCLUDE_GPS
void EnvironmentSensorManager::initBasicGPS() {

  Serial1.setPins(PIN_GPS_TX, PIN_GPS_RX);

  #ifdef GPS_BAUD_RATE
  Serial1.begin(GPS_BAUD_RATE);
  #else
  Serial1.begin(9600);
  #endif

  // Try to detect if GPS is physically connected to determine if we should expose the setting
  #ifdef PIN_GPS_EN
    pinMode(PIN_GPS_EN, OUTPUT);
    digitalWrite(PIN_GPS_EN, HIGH);   // Power on GPS
  #endif

  #ifndef PIN_GPS_EN
    MESH_DEBUG_PRINTLN("No GPS wake/reset pin found for this board. Continuing on...");
  #endif

  // Give GPS a moment to power up and send data
  delay(1000);

  // We'll consider GPS detected if we see any data on Serial1
  gps_detected = (Serial1.available() > 0);

  if (gps_detected) {
    MESH_DEBUG_PRINTLN("GPS detected");
    #ifdef PERSISTANT_GPS
      gps_active = true;
      return;
    #endif
  } else {
    MESH_DEBUG_PRINTLN("No GPS detected");
  }
  #ifdef PIN_GPS_EN
    digitalWrite(PIN_GPS_EN, LOW);  // Power off GPS until the setting is changed
  #endif
  gps_active = false; //Set GPS visibility off until setting is changed
}

void EnvironmentSensorManager::start_gps() {
  gps_active = true;
  #ifdef PIN_GPS_EN
    pinMode(PIN_GPS_EN, OUTPUT);
    digitalWrite(PIN_GPS_EN, HIGH);
    return;
  #endif

  MESH_DEBUG_PRINTLN("Start GPS is N/A on this board. Actual GPS state unchanged");
}

void EnvironmentSensorManager::stop_gps() {
  gps_active = false;
  #ifdef PIN_GPS_EN
    pinMode(PIN_GPS_EN, OUTPUT);
    digitalWrite(PIN_GPS_EN, LOW);
    return;
  #endif

  MESH_DEBUG_PRINTLN("Stop GPS is N/A on this board. Actual GPS state unchanged");
}

void EnvironmentSensorManager::loop() {
  static long next_gps_update = 0;

  _location->loop();

  if (millis() > next_gps_update) {
    if (gps_active && _location->isValid()) {
      node_lat = ((double)_location->getLatitude())/1000000.;
      node_lon = ((double)_location->getLongitude())/1000000.;
      MESH_DEBUG_PRINTLN("lat %f lon %f", node_lat, node_lon);
    }
    next_gps_update = millis() + 1000;
  }
}
#endif