#include <Arduino.h>
#include "target.h"
#include <helpers/sensors/MicroNMEALocationProvider.h>

TBeamS3SupremeBoard board;

#if defined(P_LORA_SCLK)
  static SPIClass spi;
  RADIO_CLASS radio = new Module(P_LORA_NSS, P_LORA_DIO_1, P_LORA_RESET, P_LORA_BUSY, spi);
#else
  RADIO_CLASS radio = new Module(P_LORA_NSS, P_LORA_DIO_1, P_LORA_RESET, P_LORA_BUSY);
#endif

WRAPPER_CLASS radio_driver(radio, board);

ESP32RTCClock fallback_clock;
AutoDiscoverRTCClock rtc_clock(fallback_clock);
MicroNMEALocationProvider nmea = MicroNMEALocationProvider(Serial1);
TbeamSupSensorManager sensors = TbeamSupSensorManager(nmea);

#ifndef LORA_CR
  #define LORA_CR      5
#endif

#ifdef HAS_GPS
static bool find_gps = false;
String gps_model = "None";
#endif

#ifdef HAS_PMU
XPowersAXP2101 *PMU = NULL;
bool pmuInterrupt;

static void setPmuFlag()
{
    pmuInterrupt = true;
}
#endif

uint32_t deviceOnline = 0x00;


void scanDevices(TwoWire *w)
{
    uint8_t err, addr;
    int nDevices = 0;
    uint32_t start = 0;

    Serial.println("I2C Devices scanning");
    for (addr = 1; addr < 127; addr++) {
        start = millis();
        w->beginTransmission(addr); delay(2);
        err = w->endTransmission();
        if (err == 0) {
            nDevices++;
            switch (addr) {
            case 0x77:
            case 0x76:
                Serial.println("\tFind BMX280 Sensor!");
                deviceOnline |= BME280_ONLINE;
                break;
            case 0x34:
                Serial.println("\tFind AXP192/AXP2101 PMU!");
                deviceOnline |= POWERMANAGE_ONLINE;
                break;
            case 0x3C:
                Serial.println("\tFind SSD1306/SH1106 dispaly!");
                deviceOnline |= DISPLAY_ONLINE;
                break;
            case 0x51:
                Serial.println("\tFind PCF8563 RTC!");
                deviceOnline |= PCF8563_ONLINE;
                break;
            case 0x1C:
                Serial.println("\tFind QMC6310 MAG Sensor!");
                deviceOnline |= QMC6310_ONLINE;
                break;
            default:
                Serial.print("\tI2C device found at address 0x");
                if (addr < 16) {
                    Serial.print("0");
                }
                Serial.print(addr, HEX);
                Serial.println(" !");
                break;
            }

        } else if (err == 4) {
            Serial.print("Unknow error at address 0x");
            if (addr < 16) {
                Serial.print("0");
            }
            Serial.println(addr, HEX);
        }
    }
    if (nDevices == 0)
        Serial.println("No I2C devices found\n");

    Serial.println("Scan devices done.");
    Serial.println("\n");
}

bool power_init() {
  #ifdef HAS_PMU
  if (!PMU) {
    PMU = new XPowersAXP2101(Wire1,PIN_BOARD_SDA1,PIN_BOARD_SCL1,I2C_PMU_ADD);
    if (!PMU->init()) {
      Serial.println("!! AXP2101 not found !!");
      delete PMU;
      PMU = NULL;
    } else {
      Serial.println("AXP2101 found and init");
    }
  }
  if (!PMU) {
     return false;
  }

  deviceOnline |= POWERMANAGE_ONLINE;

  PMU->setChargingLedMode(XPOWERS_CHG_LED_CTRL_CHG);

  pinMode(PIN_PMU_IRQ, INPUT_PULLUP);
  attachInterrupt(PIN_PMU_IRQ, setPmuFlag, FALLING);

  //GPS power rail
  PMU->setALDO4Voltage(3300);
  PMU->enableALDO4();

  //LoRa power rail
  PMU->setALDO3Voltage(3300);
  PMU->enableALDO3();

  //Make sure that conflicting LDOs are off
  if (ESP_SLEEP_WAKEUP_UNDEFINED == esp_sleep_get_wakeup_cause()) {
      Serial.println("Power cycle ALDO1/2 and BLDO");
      PMU->disableALDO1();
      PMU->disableALDO2();
      PMU->disableBLDO1();
      delay(250);
  }

  //BME280 and OLED
  PMU->setALDO1Voltage(3300);
  PMU->enableALDO1();

  //QMC6310U 
  PMU->setALDO2Voltage(3300);
  PMU->enableALDO2();

  //SD card power rail
  PMU->setBLDO1Voltage(3300);
  PMU->enableBLDO1();

  //Out to header pins
  PMU->setBLDO2Voltage(3300);
  PMU->enableBLDO2();

  PMU->setDC4Voltage(1200);
  PMU->enableDC4();

  PMU->setDC5Voltage(3300);
  PMU->enableDC5();

  //Other power rails
  PMU->setDC3Voltage(3300);
  PMU->enableDC3();

  //Unused power rails
  PMU->disableDC2();
  PMU->disableDLDO1();
  PMU->disableDLDO2();
  //PMU->disablePowerOutput(XPOWERS_VBACKUP);

  // Set battery charging current limit
  PMU->setChargerConstantCurr(XPOWERS_AXP2101_CHG_CUR_300MA);

  // Set battery charge max voltage
  PMU->setChargeTargetVoltage(XPOWERS_AXP2101_CHG_VOL_4V2);

  // Disable all interrupts
  PMU->disableIRQ(XPOWERS_AXP2101_ALL_IRQ);
  // Clear interrupt flags
  PMU->clearIrqStatus();
  // Enable the required interrupt function
  PMU->enableIRQ(
      XPOWERS_AXP2101_BAT_INSERT_IRQ    | XPOWERS_AXP2101_BAT_REMOVE_IRQ      |   //BATTERY
      XPOWERS_AXP2101_VBUS_INSERT_IRQ   | XPOWERS_AXP2101_VBUS_REMOVE_IRQ     |   //VBUS
      XPOWERS_AXP2101_PKEY_SHORT_IRQ    | XPOWERS_AXP2101_PKEY_LONG_IRQ       |   //POWER KEY
      XPOWERS_AXP2101_BAT_CHG_DONE_IRQ  | XPOWERS_AXP2101_BAT_CHG_START_IRQ       //CHARGE
  );
  
  //Enable voltage measurements
  PMU->enableSystemVoltageMeasure();
  PMU->enableVbusVoltageMeasure();
  PMU->enableBattVoltageMeasure();

  //Verify voltages on each rail
  Serial.printf("=========================================\n");
  if (PMU->isChannelAvailable(XPOWERS_DCDC1)) {
      Serial.printf("DC1  : %s   Voltage: %04u mV \n",  PMU->isEnableDC1()  ? "+" : "-",  PMU->getDC1Voltage());
  }
  if (PMU->isChannelAvailable(XPOWERS_DCDC2)) {
      Serial.printf("DC2  : %s   Voltage: %04u mV \n",  PMU->isEnableDC2()  ? "+" : "-",  PMU->getDC2Voltage());
  }
  if (PMU->isChannelAvailable(XPOWERS_DCDC3)) {
      Serial.printf("DC3  : %s   Voltage: %04u mV \n",  PMU->isEnableDC3()  ? "+" : "-",  PMU->getDC3Voltage());
  }
  if (PMU->isChannelAvailable(XPOWERS_DCDC4)) {
      Serial.printf("DC4  : %s   Voltage: %04u mV \n",  PMU->isEnableDC4()  ? "+" : "-",  PMU->getDC4Voltage());
  }
  if (PMU->isChannelAvailable(XPOWERS_DCDC5)) {
      Serial.printf("DC5  : %s   Voltage: %04u mV \n",  PMU->isEnableDC5()  ? "+" : "-",  PMU->getDC5Voltage());
  }
  if (PMU->isChannelAvailable(XPOWERS_ALDO1)) {
      Serial.printf("ALDO1: %s   Voltage: %04u mV \n",  PMU->isEnableALDO1()  ? "+" : "-",  PMU->getALDO1Voltage());
  }
  if (PMU->isChannelAvailable(XPOWERS_ALDO2)) {
      Serial.printf("ALDO2: %s   Voltage: %04u mV \n",  PMU->isEnableALDO2()  ? "+" : "-",  PMU->getALDO2Voltage());
  }
  if (PMU->isChannelAvailable(XPOWERS_ALDO3)) {
      Serial.printf("ALDO3: %s   Voltage: %04u mV \n",  PMU->isEnableALDO3()  ? "+" : "-",  PMU->getALDO3Voltage());
  }
  if (PMU->isChannelAvailable(XPOWERS_ALDO4)) {
      Serial.printf("ALDO4: %s   Voltage: %04u mV \n",  PMU->isEnableALDO4()  ? "+" : "-",  PMU->getALDO4Voltage());
  }
  if (PMU->isChannelAvailable(XPOWERS_BLDO1)) {
      Serial.printf("BLDO1: %s   Voltage: %04u mV \n",  PMU->isEnableBLDO1()  ? "+" : "-",  PMU->getBLDO1Voltage());
  }
  if (PMU->isChannelAvailable(XPOWERS_BLDO2)) {
      Serial.printf("BLDO2: %s   Voltage: %04u mV \n",  PMU->isEnableBLDO2()  ? "+" : "-",  PMU->getBLDO2Voltage());
  }
  Serial.printf("=========================================\n");

  //Set up and verify power button hold time
  PMU->setPowerKeyPressOffTime(XPOWERS_POWEROFF_4S);
  uint8_t opt = PMU->getPowerKeyPressOffTime();
  Serial.print("PowerKeyPressOffTime:");
  switch (opt) {
  case XPOWERS_POWEROFF_4S: Serial.println("4 Second");
      break;
  case XPOWERS_POWEROFF_6S: Serial.println("6 Second");
      break;
  case XPOWERS_POWEROFF_8S: Serial.println("8 Second");
      break;
  case XPOWERS_POWEROFF_10S: Serial.println("10 Second");
      break;
  default:
      break;
  }
#endif

  return true;
}

bool radio_init() {
  fallback_clock.begin();
  Wire1.begin(PIN_BOARD_SDA1,PIN_BOARD_SCL1);
  rtc_clock.begin(Wire1);
  
#ifdef SX126X_DIO3_TCXO_VOLTAGE
  float tcxo = SX126X_DIO3_TCXO_VOLTAGE;
#else
  float tcxo = 1.6f;
#endif

#if defined(P_LORA_SCLK)
  spi.begin(P_LORA_SCLK, P_LORA_MISO, P_LORA_MOSI);
#endif
  int status = radio.begin(LORA_FREQ, LORA_BW, LORA_SF, LORA_CR, RADIOLIB_SX126X_SYNC_WORD_PRIVATE, LORA_TX_POWER, 8, tcxo);
  if (status != RADIOLIB_ERR_NONE) {
    Serial.print("ERROR: radio init failed: ");
    Serial.println(status);
    return false;  // fail
  }

  radio.setCRC(1);
  
  return true;  // success
}

bool l76kProbe()
{
    bool result = false;
    uint32_t startTimeout ;
    Serial1.write("$PCAS03,0,0,0,0,0,0,0,0,0,0,,,0,0*02\r\n");
    delay(5);
    // Get version information
    startTimeout = millis() + 3000;
    Serial.print("Try to init L76K . Wait stop .");
    // Serial1.flush();
    while (Serial1.available()) {
        int c = Serial1.read();
        // Serial.write(c);
        // Serial.print(".");
        // Serial.flush();
        // Serial1.flush();
        if (millis() > startTimeout) {
            Serial.println("Wait L76K stop NMEA timeout!");
            return false;
        }
    };
    Serial.println();
    Serial1.flush();
    delay(200);

    Serial1.write("$PCAS06,0*1B\r\n");
    startTimeout = millis() + 500;
    String ver = "";
    while (!Serial1.available()) {
        if (millis() > startTimeout) {
            Serial.println("Get L76K timeout!");
            return false;
        }
    }
    Serial1.setTimeout(10);
    ver = Serial1.readStringUntil('\n');
    if (ver.startsWith("$GPTXT,01,01,02")) {
        Serial.println("L76K GNSS init succeeded, using L76K GNSS Module\n");
        result = true;
    }
    delay(500);

    // Initialize the L76K Chip, use GPS + GLONASS
    Serial1.write("$PCAS04,5*1C\r\n");
    delay(250);
    // only ask for RMC and GGA
    Serial1.write("$PCAS03,1,0,0,0,1,0,0,0,0,0,,,0,0*02\r\n");
    delay(250);
    // Switch to Vehicle Mode, since SoftRF enables Aviation < 2g
    Serial1.write("$PCAS11,3*1E\r\n");
    return result;
}

void TbeamSupSensorManager::start_gps()
{
  gps_active = true;
  pinMode(P_GPS_WAKE, OUTPUT);
  digitalWrite(P_GPS_WAKE, HIGH);
}

void TbeamSupSensorManager::sleep_gps() {
  gps_active = false;
  pinMode(P_GPS_WAKE, OUTPUT);
  digitalWrite(P_GPS_WAKE, LOW);
}

bool TbeamSupSensorManager::begin() {
  // init GPS port

  Serial1.begin(GPS_BAUD_RATE, SERIAL_8N1, P_GPS_RX, P_GPS_TX);

  bool result = false;
    for ( int i = 0; i < 3; ++i) {
      result = l76kProbe();
      if (result) {
        gps_active = true;
        return result;
      }
    }
  return result;
}

bool TbeamSupSensorManager::querySensors(uint8_t requester_permissions, CayenneLPP& telemetry) {
  if (requester_permissions & TELEM_PERM_LOCATION) {   // does requester have permission?
    telemetry.addGPS(TELEM_CHANNEL_SELF, node_lat, node_lon, 0.0f);
  }
  return true;
}

void TbeamSupSensorManager::loop() {
  static long next_gps_update = 0;

  _nmea->loop();

  if (millis() > next_gps_update) {
    if (_nmea->isValid()) {
      node_lat = ((double)_nmea->getLatitude())/1000000.;
      node_lon = ((double)_nmea->getLongitude())/1000000.;
      //Serial.printf("lat %f lon %f\r\n", _lat, _lon);
    }
    next_gps_update = millis() + 1000;
  }
}

int TbeamSupSensorManager::getNumSettings() const { return 1; }  // just one supported: "gps" (power switch)

const char* TbeamSupSensorManager::getSettingName(int i) const {
  return i == 0 ? "gps" : NULL;
}

const char* TbeamSupSensorManager::getSettingValue(int i) const {
  if (i == 0) {
    return gps_active ? "1" : "0";
  }
  return NULL;
}

bool TbeamSupSensorManager::setSettingValue(const char* name, const char* value) {
  if (strcmp(name, "gps") == 0) {
    if (strcmp(value, "0") == 0) {
      sleep_gps();
    } else {
      start_gps();
    }
    return true;
  }
  return false;  // not supported
}


uint16_t getBattPercent() {
  //Read the PMU fuel guage for battery %
  uint16_t battPercent = PMU->getBatteryPercent();

  return battPercent;
}

uint16_t getBattMilliVolts(){
  PMU->enableBattVoltageMeasure();
  uint16_t batVolt = PMU->getBattVoltage();
  PMU->disableBattVoltageMeasure();
  Serial.println("Battery Voltage: "+ batVolt);
  return batVolt;
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
