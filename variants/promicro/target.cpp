#include <Arduino.h>
#include "target.h"
#include <helpers/ArduinoHelpers.h>
#include <helpers/sensors/MicroNMEALocationProvider.h>

PromicroBoard board;

RADIO_CLASS radio = new Module(P_LORA_NSS, P_LORA_DIO_1, P_LORA_RESET, P_LORA_BUSY, SPI);

WRAPPER_CLASS radio_driver(radio, board);

VolatileRTCClock fallback_clock;
AutoDiscoverRTCClock rtc_clock(fallback_clock);
MicroNMEALocationProvider nmea = MicroNMEALocationProvider(Serial1);
PromicroSensorManager sensors = PromicroSensorManager(nmea);

#ifdef DISPLAY_CLASS
  DISPLAY_CLASS display;
#endif

#ifndef LORA_CR
  #define LORA_CR      5
#endif

bool radio_init() {
  rtc_clock.begin(Wire);
  
#ifdef SX126X_DIO3_TCXO_VOLTAGE
  float tcxo = SX126X_DIO3_TCXO_VOLTAGE;
#else
  float tcxo = 1.6f;
#endif

  SPI.setPins(P_LORA_MISO, P_LORA_SCLK, P_LORA_MOSI);
  SPI.begin();
  radio.setRfSwitchPins(SX126X_RXEN, SX126X_TXEN);
  int status = radio.begin(LORA_FREQ, LORA_BW, LORA_SF, LORA_CR, RADIOLIB_SX126X_SYNC_WORD_PRIVATE, LORA_TX_POWER, 8, tcxo);
  if (status == RADIOLIB_ERR_SPI_CMD_FAILED || status == RADIOLIB_ERR_SPI_CMD_INVALID) {
    #define SX126X_DIO3_TCXO_VOLTAGE (0.0f);
    tcxo = SX126X_DIO3_TCXO_VOLTAGE;
    status = radio.begin(LORA_FREQ, LORA_BW, LORA_SF, LORA_CR, RADIOLIB_SX126X_SYNC_WORD_PRIVATE, LORA_TX_POWER, 8, tcxo);
  }
  if (status != RADIOLIB_ERR_NONE) {
    Serial.print("ERROR: radio init failed: ");
    Serial.println(status);
    return false;  // fail
  }
  
  radio.setCRC(1);
  
#ifdef SX126X_CURRENT_LIMIT
  radio.setCurrentLimit(SX126X_CURRENT_LIMIT);
#endif
#ifdef SX126X_DIO2_AS_RF_SWITCH
  radio.setDio2AsRfSwitch(SX126X_DIO2_AS_RF_SWITCH);
#endif
#ifdef SX126X_RX_BOOSTED_GAIN
  radio.setRxBoostedGainMode(SX126X_RX_BOOSTED_GAIN);
#endif

  return true;  // success
}

uint32_t radio_get_rng_seed() {
  return radio.random(0x7FFFFFFF);
}

void radio_set_params(float freq, float bw, uint8_t sf, uint8_t cr) {
  radio.setFrequency(freq);
  radio.setSpreadingFactor(sf);
  radio.setBandwidth(bw);
  radio.setCodingRate(cr);
}

void radio_set_tx_power(uint8_t dbm) {
  radio.setOutputPower(dbm);
}

mesh::LocalIdentity radio_new_identity() {
  RadioNoiseListener rng(radio);
  return mesh::LocalIdentity(&rng);  // create new random identity
}

static INA3221 INA_3221(TELEM_INA3221_ADDRESS, &Wire);
static INA219 INA_219(TELEM_INA219_ADDRESS, &Wire);
static Adafruit_AHTX0 AHTX;

bool PromicroSensorManager::begin() {
  initINA3221();
  initINA219();
  initAHTX();
  initGPS();

  return true;
}

bool PromicroSensorManager::querySensors(uint8_t requester_permissions, CayenneLPP& telemetry) {
  int nextAvalableChannel = TELEM_CHANNEL_SELF + 1;
  if (requester_permissions & TELEM_PERM_ENVIRONMENT) {
    if (INA3221initialized) {
      for(int i = 0; i < 3; i++) {
        // add only enabled INA3221 channels to telemetry
        if (INA3221_CHANNEL_ENABLED[i]) {
          telemetry.addVoltage(nextAvalableChannel, INA_3221.getBusVoltage(i));
          telemetry.addCurrent(nextAvalableChannel, INA_3221.getCurrent(i));
          telemetry.addPower(nextAvalableChannel, INA_3221.getPower(i));
          nextAvalableChannel++;
        }
      }
    }
    if (INA219initialized) {
      telemetry.addVoltage(nextAvalableChannel, INA_219.getBusVoltage());
      telemetry.addCurrent(nextAvalableChannel, INA_219.getCurrent());
      telemetry.addPower(nextAvalableChannel, INA_219.getPower());
      nextAvalableChannel++;
    }
    if (AHTXinitialized) {
      sensors_event_t humidity, temp;
      AHTX.getEvent(&humidity, &temp);
      telemetry.addTemperature(TELEM_CHANNEL_SELF, temp.temperature);
      telemetry.addRelativeHumidity(TELEM_CHANNEL_SELF, humidity.relative_humidity);
    }
  }
  if (requester_permissions & TELEM_PERM_LOCATION) {   // does requester have permission?
    telemetry.addGPS(TELEM_CHANNEL_SELF, node_lat, node_lon, 0.0f);
  }
  return true;
}

int PromicroSensorManager::getNumSettings() const {
  return NUM_SENSOR_SETTINGS;
}

const char* PromicroSensorManager::getSettingName(int i) const {
  if (gps_detected && i == 0) return "gps";
  if (i >= 0 && i < NUM_SENSOR_SETTINGS) {
    return INA3221_CHANNEL_NAMES[i];
  }
  return NULL;
}

const char* PromicroSensorManager::getSettingValue(int i) const {
  if (gps_detected && i == 0) {
    return gps_active ? "1" : "0";
  }
  if (i >= 0 && i < NUM_SENSOR_SETTINGS) {
    return INA3221_CHANNEL_ENABLED[i] ? "1" : "0";
  }
  return NULL;
}

bool PromicroSensorManager::setSettingValue(const char* name, const char* value) {
  if (gps_detected && strcmp(name, "gps") == 0) {
    if (strcmp(value, "0") == 0) {
      stop_gps();
    } else {
      start_gps();
    }
    return true;
  }
  for (int i = 0; i < NUM_SENSOR_SETTINGS; i++) {
    if (strcmp(name, INA3221_CHANNEL_NAMES[i]) == 0) {
      int channelEnabled = INA_3221.getEnableChannel(i);
      if (strcmp(value, "1") == 0) {
        INA3221_CHANNEL_ENABLED[i] = true;
        if (!channelEnabled) {
          INA_3221.enableChannel(i);
        }
      } else {
        INA3221_CHANNEL_ENABLED[i] = false;
        if (channelEnabled) {
          INA_3221.disableChannel(i);
        }
      }
      return true;
    }
  }
  return false;
}

void PromicroSensorManager::initINA3221() {
  if (INA_3221.begin()) {
    MESH_DEBUG_PRINTLN("Found INA3221 at address: %02X", INA_3221.getAddress());
    MESH_DEBUG_PRINTLN("%04X %04X %04X", INA_3221.getDieID(), INA_3221.getManufacturerID(), INA_3221.getConfiguration());

    for(int i = 0; i < 3; i++) {
      INA_3221.setShuntR(i, TELEM_INA3221_SHUNT_VALUE);
    }
    INA3221initialized = true;
  } else {
    INA3221initialized = false;
    MESH_DEBUG_PRINTLN("INA3221 was not found at I2C address %02X", TELEM_INA3221_ADDRESS);
  }
}

void PromicroSensorManager::initINA219() { 
  if (INA_219.begin()) {
    MESH_DEBUG_PRINTLN("Found INA219 at address: %02X", INA_219.getAddress());
    INA_219.setMaxCurrentShunt(TELEM_INA219_MAX_CURRENT, TELEM_INA219_SHUNT_VALUE);
    INA219initialized = true;
  } else {
    INA219initialized = false;
    MESH_DEBUG_PRINTLN("INA219 was not found at I2C address %02X", TELEM_INA219_ADDRESS);
  }
}

void PromicroSensorManager::initAHTX() { 
  if (AHTX.begin(&Wire, 0, TELEM_AHTX_ADDRESS)) {
    MESH_DEBUG_PRINTLN("Found AHT10/AHT20 at address: %02X", TELEM_AHTX_ADDRESS);
    AHTXinitialized = true;
  } else {
    AHTXinitialized = false;
    MESH_DEBUG_PRINTLN("AHT10/AHT20 was not found at I2C address %02X", TELEM_AHTX_ADDRESS);
  }
}

void PromicroSensorManager::initGPS() {
  Serial1.setPins(PIN_GPS_TX, PIN_GPS_RX);
  Serial1.begin(9600);

  // Try to detect if GPS is physically connected to determine if we should expose the setting
  pinMode(PIN_GPS_EN, OUTPUT);
  digitalWrite(PIN_GPS_EN, HIGH);  // Power on GPS

  // Give GPS a moment to power up and send data
  delay(1000);

  // We'll consider GPS detected if we see any data on Serial1
  gps_detected = (Serial1.available() > 0);

  if (gps_detected) {
    MESH_DEBUG_PRINTLN("GPS detected");
    digitalWrite(PIN_GPS_EN, LOW);  // Power off GPS until the setting is changed
  } else {
    MESH_DEBUG_PRINTLN("No GPS detected");
    digitalWrite(PIN_GPS_EN, LOW);
  }

}


void PromicroSensorManager::start_gps() {
  gps_active = true;
  pinMode(PIN_GPS_EN, OUTPUT);
  digitalWrite(PIN_GPS_EN, HIGH);
}

void PromicroSensorManager::stop_gps() {
  gps_active = false;
  pinMode(PIN_GPS_EN, OUTPUT);
  digitalWrite(PIN_GPS_EN, LOW);
}

void PromicroSensorManager::loop() {
  static long next_gps_update = 0;

  _location->loop();

  if (millis() > next_gps_update) {
    if (_location->isValid()) {
      node_lat = ((double)_location->getLatitude())/1000000.;
      node_lon = ((double)_location->getLongitude())/1000000.;
      MESH_DEBUG_PRINTLN("lat %f lon %f", node_lat, node_lon);
    }
    next_gps_update = millis() + 1000;
  }
}