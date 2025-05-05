#include <Arduino.h>
#include "target.h"

TBeamS3SupremeBoard board;

// Using PMU AXP2102
XPowersAXP2101 PMU;

bool pmuIntFlag;

#ifndef LORA_CR
  #define LORA_CR      5
#endif

#if defined(P_LORA_SCLK)
  static SPIClass spi;
  RADIO_CLASS radio = new Module(P_LORA_NSS, P_LORA_DIO_1, P_LORA_RESET, P_LORA_BUSY, spi);
#else
  RADIO_CLASS radio = new Module(P_LORA_NSS, P_LORA_DIO_1, P_LORA_RESET, P_LORA_BUSY);
#endif

WRAPPER_CLASS radio_driver(radio, board);

ESP32RTCClock fallback_clock;
AutoDiscoverRTCClock rtc_clock(fallback_clock);
SensorManager sensors;

static void setPMUIntFlag(){
  pmuIntFlag = true;
}

bool power_init() {
  //Start up Wire1 with PMU address
  //Serial.println("Starting Wire1 for PMU");
  //Wire1.begin(I2C_PMU_ADD);
  //Wire1.begin(PIN_BOARD_SDA1,PIN_BOARD_SCL1);
  
  //Set LED to indicate charge state
  Serial.println("Setting charge led");
  PMU.setChargingLedMode(XPOWERS_CHG_LED_CTRL_CHG);  
  
  //Set up PMU interrupts
  Serial.println("Setting up PMU interrupts");
  pinMode(PIN_PMU_IRQ,INPUT_PULLUP);
  attachInterrupt(PIN_PMU_IRQ,setPMUIntFlag,FALLING);

  //GPS
  Serial.println("Setting and enabling a-ldo4 for GPS");
  PMU.setALDO4Voltage(3300);  
  PMU.enableALDO4();          //disable to save power
  
  //Lora
  Serial.println("Setting and enabling a-ldo3 for LoRa");
  PMU.setALDO3Voltage(3300);  
  PMU.enableALDO3();   

  //To avoid SPI bus issues during power up, reset OLED, sensor, and SD card supplies
  Serial.println("Reset a-ldo1&2 and b-ldo1");
  if(ESP_SLEEP_WAKEUP_UNDEFINED == esp_sleep_get_wakeup_cause()){
    PMU.enableALDO1();
    PMU.enableALDO2();
    PMU.enableBLDO1();
    delay(250);
  }
 
  //BME280 and OLED
  Serial.println("Setting and enabling a-ldo1 for oled");
  PMU.setALDO1Voltage(3300);
  PMU.enableALDO1();

  //QMC6310U 
  Serial.println("Setting and enabling a-ldo2 for QMC");
  PMU.setALDO2Voltage(3300);
  PMU.enableALDO2();          //disable to save power

  //SD card
  Serial.println("Setting and enabling b-ldo2 for SD card");
  PMU.setBLDO1Voltage(3300);
  PMU.enableBLDO1();

  //Out to header pins
  Serial.println("Setting and enabling b-ldo2 for output to header");
  PMU.setBLDO2Voltage(3300);
  PMU.enableBLDO2();

  Serial.println("Setting and enabling dcdc4 for output to header");
  PMU.setDC4Voltage(XPOWERS_AXP2101_DCDC4_VOL2_MAX);    //1.8V
  PMU.enableDC4();

  Serial.println("Setting and enabling dcdc5 for output to header");
  PMU.setDC5Voltage(3300);
  PMU.enableDC5();

  //Other power rails
  Serial.println("Setting and enabling dcdc3 for ?");
  PMU.setDC3Voltage(3300);    //doesn't go anywhere in the schematic??
  PMU.enableDC3();

  //Unused power rails
  Serial.println("Disabling unused supplies dcdc2, dldo1 and dldo2");
  PMU.disableDC2();
  PMU.disableDLDO1();
  PMU.disableDLDO2();  

  //Set charge current to 300mA
  Serial.println("Setting battery charge current limit and voltage");
  PMU.setChargerConstantCurr(XPOWERS_AXP2101_CHG_CUR_300MA);
  PMU.setChargeTargetVoltage(XPOWERS_AXP2101_CHG_VOL_4V2);

  //enable battery voltage measurement
  Serial.println("Enabling battery measurement");
  PMU.enableBattVoltageMeasure();

  //Reset and re-enable PMU interrupts
  Serial.println("Re-enable interrupts");
  PMU.disableIRQ(XPOWERS_AXP2101_ALL_IRQ);
  PMU.clearIrqStatus();
  PMU.enableIRQ(
    XPOWERS_AXP2101_BAT_INSERT_IRQ    | XPOWERS_AXP2101_BAT_REMOVE_IRQ      |   //Battery interrupts
    XPOWERS_AXP2101_VBUS_INSERT_IRQ   | XPOWERS_AXP2101_VBUS_REMOVE_IRQ     |   //VBUS interrupts
    XPOWERS_AXP2101_PKEY_SHORT_IRQ    | XPOWERS_AXP2101_PKEY_LONG_IRQ       |   //Power Key interrupts
    XPOWERS_AXP2101_BAT_CHG_DONE_IRQ  | XPOWERS_AXP2101_BAT_CHG_START_IRQ       //Charging interrupts
  );

  //Set the power key off press time
  PMU.setPowerKeyPressOffTime(XPOWERS_POWEROFF_4S);

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

uint16_t getBattPercent() {
  //Read the PMU fuel guage for battery %
  uint16_t battPercent = PMU.getBatteryPercent();

  return battPercent;
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
