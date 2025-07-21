#include "EnvironmentSensorManager.h"

#if ENV_PIN_SDA && ENV_PIN_SCL
#define TELEM_WIRE &Wire1  // Use Wire1 as the I2C bus for Environment Sensors
#else
#define TELEM_WIRE &Wire  // Use default I2C bus for Environment Sensors
#endif

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

#if ENV_INCLUDE_BME680
#ifndef TELEM_BME680_ADDRESS
#define TELEM_BME680_ADDRESS    0x76      // BME680 environmental sensor I2C address
#endif
static Bsec2 BME680;
#define SAMPLING_RATE		BSEC_SAMPLE_RATE_ULP
static float rawPressure = 0;
static float rawTemperature = 0;
static float compTemperature = 0;
static float rawHumidity = 0;
static float compHumidity = 0;
static float readIAQ = 0;
static float readStaticIAQ = 0;
static float readCO2 = 0;
#endif

#if ENV_INCLUDE_BMP280
#ifndef TELEM_BMP280_ADDRESS
#define TELEM_BMP280_ADDRESS    0x76      // BMP280 environmental sensor I2C address
#endif
#define TELEM_BMP280_SEALEVELPRESSURE_HPA (1013.25)    // Athmospheric pressure at sea level
#include <Adafruit_BMP280.h>
static Adafruit_BMP280 BMP280;
#endif

#if ENV_INCLUDE_SHTC3
#include <Adafruit_SHTC3.h>
static Adafruit_SHTC3 SHTC3;
#endif

#if ENV_INCLUDE_LPS22HB
#include <Arduino_LPS22HB.h>
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

#if ENV_INCLUDE_MLX90614
#define TELEM_MLX90614_ADDRESS 0x5A      // MLX90614 IR temperature sensor I2C address
#include <Adafruit_MLX90614.h>
static Adafruit_MLX90614 MLX90614;
#endif

#if ENV_INCLUDE_VL53L0X
#define TELEM_VL53L0X_ADDRESS 0x29      // VL53L0X time-of-flight distance sensor I2C address
#include <Adafruit_VL53L0X.h>
static Adafruit_VL53L0X VL53L0X;
#endif

#if ENV_INCLUDE_GPS && RAK_BOARD
static uint32_t gpsResetPin = 0;
static bool i2cGPSFlag = false;
static bool serialGPSFlag = false;
#define TELEM_RAK12500_ADDRESS   0x42     //RAK12500 Ublox GPS via i2c
#include <SparkFun_u-blox_GNSS_Arduino_Library.h>
static SFE_UBLOX_GNSS ublox_GNSS;
#endif

bool EnvironmentSensorManager::begin() {
  #if ENV_INCLUDE_GPS
  #if RAK_BOARD
  rakGPSInit();   //probe base board/sockets for GPS
  #else
  initBasicGPS();
  #endif
  #endif

  #if ENV_PIN_SDA && ENV_PIN_SCL
  Wire1.begin(ENV_PIN_SDA, ENV_PIN_SCL, 100000);
  MESH_DEBUG_PRINTLN("Second I2C initialized on pins SDA: %d SCL: %d", ENV_PIN_SDA, ENV_PIN_SCL);
  #endif

  #if ENV_INCLUDE_AHTX0
  if (AHTX0.begin(TELEM_WIRE, 0, TELEM_AHTX_ADDRESS)) {
    MESH_DEBUG_PRINTLN("Found AHT10/AHT20 at address: %02X", TELEM_AHTX_ADDRESS);
    AHTX0_initialized = true;
  } else {
    AHTX0_initialized = false;
    MESH_DEBUG_PRINTLN("AHT10/AHT20 was not found at I2C address %02X", TELEM_AHTX_ADDRESS);
  }
  #endif

  #if ENV_INCLUDE_BME280
  if (BME280.begin(TELEM_BME280_ADDRESS, TELEM_WIRE)) {
    MESH_DEBUG_PRINTLN("Found BME280 at address: %02X", TELEM_BME280_ADDRESS);
    MESH_DEBUG_PRINTLN("BME sensor ID: %02X", BME280.sensorID());
    BME280_initialized = true;
  } else {
    BME280_initialized = false;
    MESH_DEBUG_PRINTLN("BME280 was not found at I2C address %02X", TELEM_BME280_ADDRESS);
  }
  #endif

  #if ENV_INCLUDE_BME680
  bsecSensor sensorList[5] = {
    BSEC_OUTPUT_IAQ,
  //  BSEC_OUTPUT_RAW_TEMPERATURE,
    BSEC_OUTPUT_RAW_PRESSURE,
  //  BSEC_OUTPUT_RAW_HUMIDITY,
  //  BSEC_OUTPUT_RAW_GAS,
  //  BSEC_OUTPUT_STABILIZATION_STATUS,
  //  BSEC_OUTPUT_RUN_IN_STATUS,
    BSEC_OUTPUT_SENSOR_HEAT_COMPENSATED_TEMPERATURE,
    BSEC_OUTPUT_SENSOR_HEAT_COMPENSATED_HUMIDITY,
  //  BSEC_OUTPUT_STATIC_IAQ,
    BSEC_OUTPUT_CO2_EQUIVALENT,
  //  BSEC_OUTPUT_BREATH_VOC_EQUIVALENT,
  //  BSEC_OUTPUT_GAS_PERCENTAGE,
  //  BSEC_OUTPUT_COMPENSATED_GAS
  };

  if(!BME680.begin(TELEM_BME680_ADDRESS, Wire)){
    checkBMEStatus(BME680);
    BME680_initialized = false;
    return false;
  }
    
  MESH_DEBUG_PRINTLN("Found BME680 at address: %02X", TELEM_BME680_ADDRESS);
  BME680_initialized = true;

  if (SAMPLING_RATE == BSEC_SAMPLE_RATE_ULP)
	{
	  BME680.setTemperatureOffset(BSEC_SAMPLE_RATE_ULP);
	}
  else if (SAMPLING_RATE == BSEC_SAMPLE_RATE_LP)
	{
	  BME680.setTemperatureOffset(TEMP_OFFSET_LP);
	}

  if (!BME680.updateSubscription(sensorList, ARRAY_LEN(sensorList), SAMPLING_RATE))
  {
    checkBMEStatus(BME680);
  }

  BME680.attachCallback(newDataCallback);
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

  #if ENV_INCLUDE_SHTC3
  if (SHTC3.begin()) {
    MESH_DEBUG_PRINTLN("Found sensor: SHTC3");
    SHTC3_initialized = true;
  } else {
    SHTC3_initialized = false;
    MESH_DEBUG_PRINTLN("SHTC3 was not found at I2C address %02X", 0x70);
  }
  #endif

  #if ENV_INCLUDE_LPS22HB
  if (BARO.begin()) {
    MESH_DEBUG_PRINTLN("Found sensor: LPS22HB");
    LPS22HB_initialized = true;
  } else {
    LPS22HB_initialized = false;
    MESH_DEBUG_PRINTLN("LPS22HB was not found at I2C address %02X", 0x5C);
  }
  #endif

  #if ENV_INCLUDE_INA3221
  if (INA3221.begin(TELEM_INA3221_ADDRESS, TELEM_WIRE)) {
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
  if (INA219.begin(TELEM_WIRE)) {
    MESH_DEBUG_PRINTLN("Found INA219 at address: %02X", TELEM_INA219_ADDRESS);
    INA219_initialized = true;
  } else {
    INA219_initialized = false;
    MESH_DEBUG_PRINTLN("INA219 was not found at I2C address %02X", TELEM_INA219_ADDRESS);
  }
  #endif

  #if ENV_INCLUDE_MLX90614
  if (MLX90614.begin(TELEM_MLX90614_ADDRESS, TELEM_WIRE)) {
    MESH_DEBUG_PRINTLN("Found MLX90614 at address: %02X", TELEM_MLX90614_ADDRESS);
    MLX90614_initialized = true;
  } else {
    MLX90614_initialized = false;
    MESH_DEBUG_PRINTLN("MLX90614 was not found at I2C address %02X", TELEM_MLX90614_ADDRESS);
  }
  #endif

  #if ENV_INCLUDE_VL53L0X
  if (VL53L0X.begin(TELEM_VL53L0X_ADDRESS, false, TELEM_WIRE)) {
    MESH_DEBUG_PRINTLN("Found VL53L0X at address: %02X", TELEM_VL53L0X_ADDRESS);
    VL53L0X_initialized = true;
  } else {
    VL53L0X_initialized = false;
    MESH_DEBUG_PRINTLN("VL53L0X was not found at I2C address %02X", TELEM_VL53L0X_ADDRESS);
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
      telemetry.addBarometricPressure(TELEM_CHANNEL_SELF, BME280.readPressure()/100);
      telemetry.addAltitude(TELEM_CHANNEL_SELF, BME280.readAltitude(TELEM_BME280_SEALEVELPRESSURE_HPA));
    }
    #endif

    #if ENV_INCLUDE_BME680
    if (BME680_initialized) {
      telemetry.addTemperature(TELEM_CHANNEL_SELF, compTemperature);
      telemetry.addRelativeHumidity(TELEM_CHANNEL_SELF, compHumidity);
      telemetry.addBarometricPressure(TELEM_CHANNEL_SELF, rawPressure);
      telemetry.addGenericSensor(TELEM_CHANNEL_SELF+1, readIAQ);
      telemetry.addConcentration(TELEM_CHANNEL_SELF+1, readCO2);
    }
    #endif

    #if ENV_INCLUDE_BMP280
    if (BMP280_initialized) {
      telemetry.addTemperature(TELEM_CHANNEL_SELF, BMP280.readTemperature());
      telemetry.addBarometricPressure(TELEM_CHANNEL_SELF, BMP280.readPressure()/100);
      telemetry.addAltitude(TELEM_CHANNEL_SELF, BME280.readAltitude(TELEM_BME280_SEALEVELPRESSURE_HPA));
    }
    #endif

    #if ENV_INCLUDE_SHTC3
    if (SHTC3_initialized) {
      sensors_event_t humidity, temp;
      SHTC3.getEvent(&humidity, &temp);

      telemetry.addTemperature(TELEM_CHANNEL_SELF, temp.temperature);
      telemetry.addRelativeHumidity(TELEM_CHANNEL_SELF, humidity.relative_humidity);
    }
    #endif

    #if ENV_INCLUDE_LPS22HB
    if (LPS22HB_initialized) {
      telemetry.addTemperature(TELEM_CHANNEL_SELF, BARO.readTemperature());
      telemetry.addBarometricPressure(TELEM_CHANNEL_SELF, BARO.readPressure());
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

    #if ENV_INCLUDE_MLX90614
    if (MLX90614_initialized) {
      telemetry.addTemperature(TELEM_CHANNEL_SELF, MLX90614.readObjectTempC());
      telemetry.addTemperature(TELEM_CHANNEL_SELF + 1, MLX90614.readAmbientTempC());
    }
    #endif

    #if ENV_INCLUDE_VL53L0X
    if (VL53L0X_initialized) {
      VL53L0X_RangingMeasurementData_t measure;
      VL53L0X.rangingTest(&measure, false); // pass in 'true' to get debug data
      if (measure.RangeStatus != 4) { // phase failures
        telemetry.addDistance(TELEM_CHANNEL_SELF, measure.RangeMilliMeter / 1000.0f); // convert mm to m
      } else {
        telemetry.addDistance(TELEM_CHANNEL_SELF, 0.0f); // no valid measurement
      }
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

#if ENV_INCLUDE_BME680
void EnvironmentSensorManager::checkBMEStatus(Bsec2 bsec) {
  if (bsec.status < BSEC_OK)
  {
    MESH_DEBUG_PRINTLN("BSEC error code : %f", float(bsec.status));
  }
  else if (bsec.status > BSEC_OK)
  {
    MESH_DEBUG_PRINTLN("BSEC warning code : %f", float(bsec.status));
  }

  if (bsec.sensor.status < BME68X_OK)
  {
    MESH_DEBUG_PRINTLN("BME68X error code : %f", bsec.sensor.status);
  }
  else if (bsec.sensor.status > BME68X_OK)
  {
    MESH_DEBUG_PRINTLN("BME68X warning code : %f", bsec.sensor.status);
  }
}

void EnvironmentSensorManager::newDataCallback(const bme68xData data, const bsecOutputs outputs, Bsec2 bsec) {
  if (!outputs.nOutputs) {
    MESH_DEBUG_PRINTLN("No new data to report out");
    return;
  }

  MESH_DEBUG_PRINTLN("BSEC outputs:\n\tTime stamp = %f", (int) (outputs.output[0].time_stamp / INT64_C(1000000)));
  for (uint8_t i = 0; i < outputs.nOutputs; i++) {
    const bsecData output  = outputs.output[i];
    switch (output.sensor_id)
    {
      case BSEC_OUTPUT_IAQ:
        readIAQ = output.signal;
        MESH_DEBUG_PRINTLN("\tIAQ = %f", output.signal);
        MESH_DEBUG_PRINTLN("\tIAQ accuracy = %f", output.accuracy);
        break;
      case BSEC_OUTPUT_RAW_TEMPERATURE:
        rawTemperature = output.signal;
        MESH_DEBUG_PRINTLN("\tTemperature = %f", output.signal);
        break;
      case BSEC_OUTPUT_RAW_PRESSURE:
        rawPressure = output.signal;
        MESH_DEBUG_PRINTLN("\tPressure = %f", output.signal);
        break;
      case BSEC_OUTPUT_RAW_HUMIDITY:
        rawHumidity = output.signal;
        MESH_DEBUG_PRINTLN("\tHumidity = %f", output.signal);
        break;
      case BSEC_OUTPUT_RAW_GAS:
        MESH_DEBUG_PRINTLN("\tGas resistance = %f", output.signal);
        break;
      case BSEC_OUTPUT_STABILIZATION_STATUS:
        MESH_DEBUG_PRINTLN("\tStabilization status = %f", output.signal);
        break;
      case BSEC_OUTPUT_RUN_IN_STATUS:
        MESH_DEBUG_PRINTLN("\tRun in status = %f", output.signal);
        break;
      case BSEC_OUTPUT_SENSOR_HEAT_COMPENSATED_TEMPERATURE:
        compTemperature = output.signal;
        MESH_DEBUG_PRINTLN("\tCompensated temperature = %f", output.signal);
        break;
      case BSEC_OUTPUT_SENSOR_HEAT_COMPENSATED_HUMIDITY:
        compHumidity = output.signal;
        MESH_DEBUG_PRINTLN("\tCompensated humidity = %f", output.signal);
        break;
      case BSEC_OUTPUT_STATIC_IAQ:
        readStaticIAQ = output.signal;
        MESH_DEBUG_PRINTLN("\tStatic IAQ = %f", output.signal);
        break;
      case BSEC_OUTPUT_CO2_EQUIVALENT:
        readCO2 = output.signal;
        MESH_DEBUG_PRINTLN("\tCO2 Equivalent = %f", output.signal);
        break;
      case BSEC_OUTPUT_BREATH_VOC_EQUIVALENT:
        MESH_DEBUG_PRINTLN("\tbVOC equivalent = %f", output.signal);
        break;
      case BSEC_OUTPUT_GAS_PERCENTAGE:
        MESH_DEBUG_PRINTLN("\tGas percentage = %f", output.signal);
        break;
      case BSEC_OUTPUT_COMPENSATED_GAS:
        MESH_DEBUG_PRINTLN("\tCompensated gas = %f", output.signal);
        break;
      default:
        break;
    }
  }
}
#endif

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

#ifdef RAK_BOARD
void EnvironmentSensorManager::rakGPSInit(){

  Serial1.setPins(PIN_GPS_TX, PIN_GPS_RX);

  #ifdef GPS_BAUD_RATE
  Serial1.begin(GPS_BAUD_RATE);
  #else
  Serial1.begin(9600);
  #endif

  //search for the correct IO standby pin depending on socket used
  if(gpsIsAwake(WB_IO2)){
  //  MESH_DEBUG_PRINTLN("RAK base board is RAK19007/10");
  //  MESH_DEBUG_PRINTLN("GPS is installed on Socket A");
  }
  else if(gpsIsAwake(WB_IO4)){
  //  MESH_DEBUG_PRINTLN("RAK base board is RAK19003/9");
  //  MESH_DEBUG_PRINTLN("GPS is installed on Socket C");
  }
  else if(gpsIsAwake(WB_IO5)){
  //  MESH_DEBUG_PRINTLN("RAK base board is RAK19001/11");
  //  MESH_DEBUG_PRINTLN("GPS is installed on Socket F");
  }
  else{
    MESH_DEBUG_PRINTLN("No GPS found");
    gps_active = false;
    gps_detected = false;
    return;
  }

  #ifndef FORCE_GPS_ALIVE // for use with repeaters, until GPS toggle is implimented
  //Now that GPS is found and set up, set to sleep for initial state
  stop_gps();
  #endif
}

bool EnvironmentSensorManager::gpsIsAwake(uint8_t ioPin){

  //set initial waking state
  pinMode(ioPin,OUTPUT);
  digitalWrite(ioPin,LOW);
  delay(500);
  digitalWrite(ioPin,HIGH);
  delay(500);

  //Try to init RAK12500 on I2C
  if (ublox_GNSS.begin(Wire) == true){
    MESH_DEBUG_PRINTLN("RAK12500 GPS init correctly with pin %i",ioPin);
    ublox_GNSS.setI2COutput(COM_TYPE_NMEA);
    ublox_GNSS.saveConfigSelective(VAL_CFG_SUBSEC_IOPORT);
    gpsResetPin = ioPin;
    i2cGPSFlag = true;
    gps_active = true;
    gps_detected = true;
    return true;
  }
  else if(Serial1){
    MESH_DEBUG_PRINTLN("Serial GPS init correctly and is turned on");
    if(PIN_GPS_EN){
      gpsResetPin = PIN_GPS_EN;
    }
    serialGPSFlag = true;
    gps_active = true;
    gps_detected = true;
    return true;
  }
  MESH_DEBUG_PRINTLN("GPS did not init with this IO pin... try the next");
  return false;
}
#endif

void EnvironmentSensorManager::start_gps() {
  gps_active = true;
  #ifdef RAK_BOARD
    pinMode(gpsResetPin, OUTPUT);
    digitalWrite(gpsResetPin, HIGH);
    return;
  #endif
  #ifdef PIN_GPS_EN
    pinMode(PIN_GPS_EN, OUTPUT);
    digitalWrite(PIN_GPS_EN, HIGH);
    return;
  #endif

  MESH_DEBUG_PRINTLN("Start GPS is N/A on this board. Actual GPS state unchanged");
}

void EnvironmentSensorManager::stop_gps() {
  gps_active = false;
  #ifdef RAK_BOARD
    pinMode(gpsResetPin, OUTPUT);
    digitalWrite(gpsResetPin, LOW);
    return;
  #endif
  #ifdef PIN_GPS_EN
    pinMode(PIN_GPS_EN, OUTPUT);
    digitalWrite(PIN_GPS_EN, LOW);
    return;
  #endif

  MESH_DEBUG_PRINTLN("Stop GPS is N/A on this board. Actual GPS state unchanged");
}
#endif

#ifndef ENV_INCLUDE_GPS && defined(ENV_INCLUDE_BME680)  //if there is no gps but there is bme680
void EnvironmentSensorManager::loop() {
  static long next_update = 0;

    if(BME680_initialized){
      if (!BME680.run()){
        checkBMEStatus(BME680);
      }
     }
    next_update = millis() + 1000;
  }
#endif
#if defined(ENV_INCLUDE_GPS) && defined(ENV_INCLUDE_BME680) //if there is both gps and bme680
void EnvironmentSensorManager::loop() {
  static long next_update = 0;

  _location->loop();

  if (millis() > next_update) {
    if(gps_active){
    #ifndef RAK_BOARD
    if (_location->isValid()) {
      node_lat = ((double)_location->getLatitude())/1000000.;
      node_lon = ((double)_location->getLongitude())/1000000.;
      MESH_DEBUG_PRINTLN("lat %f lon %f", node_lat, node_lon);
    }
    #else
    if(i2cGPSFlag){
      node_lat = ((double)ublox_GNSS.getLatitude())/10000000.;
      node_lon = ((double)ublox_GNSS.getLongitude())/10000000.;
      MESH_DEBUG_PRINTLN("lat %f lon %f", node_lat, node_lon);
    }
    else if (serialGPSFlag && _location->isValid()) {
      node_lat = ((double)_location->getLatitude())/1000000.;
      node_lon = ((double)_location->getLongitude())/1000000.;
      MESH_DEBUG_PRINTLN("lat %f lon %f", node_lat, node_lon);
    }
    #endif
    }

    if(BME680_initialized){
      if (!BME680.run()){
        checkBMEStatus(BME680);
      }
     }
    next_update = millis() + 1000;
  }
}
#endif
#ifndef ENV_INCLUDE_BME680 && defined(ENV_INCLUDE_GPS)  //if there is no bme680 but there is gps
void EnvironmentSensorManager::loop() {
  static long next_update = 0;

  _location->loop();

  if (millis() > next_update) {
    if(gps_active){
    #ifndef RAK_BOARD
    if (_location->isValid()) {
      node_lat = ((double)_location->getLatitude())/1000000.;
      node_lon = ((double)_location->getLongitude())/1000000.;
      MESH_DEBUG_PRINTLN("lat %f lon %f", node_lat, node_lon);
    }
    #else
    if(i2cGPSFlag){
      node_lat = ((double)ublox_GNSS.getLatitude())/10000000.;
      node_lon = ((double)ublox_GNSS.getLongitude())/10000000.;
      MESH_DEBUG_PRINTLN("lat %f lon %f", node_lat, node_lon);
    }
    else if (serialGPSFlag && _location->isValid()) {
      node_lat = ((double)_location->getLatitude())/1000000.;
      node_lon = ((double)_location->getLongitude())/1000000.;
      MESH_DEBUG_PRINTLN("lat %f lon %f", node_lat, node_lon);
    }
    #endif
    }
    next_update = millis() + 1000;
  }
}
#endif