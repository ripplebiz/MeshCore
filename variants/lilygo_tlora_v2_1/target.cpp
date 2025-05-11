#include <Arduino.h>
#include "target.h"

LilyGoTLoraBoard board;

static SPIClass spi;
RADIO_CLASS radio = new Module(P_LORA_NSS, P_LORA_DIO_0, P_LORA_RESET, P_LORA_DIO_1, spi);

WRAPPER_CLASS radio_driver(radio, board);

ESP32RTCClock fallback_clock;
AutoDiscoverRTCClock rtc_clock(fallback_clock);
SensorManager sensors;

#ifndef LORA_CR
  #define LORA_CR      5
#endif

bool radio_init() {
  fallback_clock.begin();
  rtc_clock.begin(Wire);

  spi.begin(P_LORA_SCLK, P_LORA_MISO, P_LORA_MOSI);
  int status = radio.begin(LORA_FREQ, LORA_BW, LORA_SF, LORA_CR, RADIOLIB_SX126X_SYNC_WORD_PRIVATE, LORA_TX_POWER, 8);
  if (status != RADIOLIB_ERR_NONE) {
    Serial.print("ERROR: radio init failed: ");
    Serial.println(status);
    return false;  // fail
  }

#ifdef SX127X_CURRENT_LIMIT
  radio.setCurrentLimit(SX127X_CURRENT_LIMIT);
#endif

  radio.setCRC(1);

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
