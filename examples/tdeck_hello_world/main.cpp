#include <Arduino.h>
#include <Wire.h>
#include <RadioLib.h>
#include "./display.h"
#include "./mesh.h"

#define LILYGO_KB_ADDR 0x55

#define BOARD_PWR 10
#define BOARD_SDA 18
#define BOARD_SCL 8

char message_buffer[20] = "Hello, World!";
uint8_t message_buffer_cursor = 0;

#if defined(NRF52_PLATFORM)
RADIO_CLASS radio = new Module(P_LORA_NSS, P_LORA_DIO_1, P_LORA_RESET, P_LORA_BUSY, SPI);
#elif defined(P_LORA_SCLK)
SPIClass spi;
RADIO_CLASS radio = new Module(P_LORA_NSS, P_LORA_DIO_1, P_LORA_RESET, P_LORA_BUSY, spi);
#else
RADIO_CLASS radio = new Module(P_LORA_NSS, P_LORA_DIO_1, P_LORA_RESET, P_LORA_BUSY);
#endif
StdRNG fast_rng;
SimpleMeshTables tables;
MyMesh the_mesh(*new WRAPPER_CLASS(radio, board), fast_rng, *new VolatileRTCClock(), tables);

void halt() {
    while (1) {
        delay(1000);
    }
}

void setup() {
    Serial.begin(115200);

#ifdef SX126X_DIO3_TCXO_VOLTAGE
    float tcxo = SX126X_DIO3_TCXO_VOLTAGE;
#else
    float tcxo = 1.6f;
#endif

#if defined(NRF52_PLATFORM)
    SPI.setPins(P_LORA_MISO, P_LORA_SCLK, P_LORA_MOSI);
    SPI.begin();
#elif defined(P_LORA_SCLK)
    spi.begin(P_LORA_SCLK, P_LORA_MISO, P_LORA_MOSI);
#endif

    int status = radio.begin(
        LORA_FREQUENCY,
        LORA_BANDWIDTH,
        LORA_SPREADING_FACTOR,
        LORA_TRANSMISSION_POWER,
        LORA_SYNC_WORD,
        tcxo
    );

    if (status != RADIOLIB_ERR_NONE) {
        Serial.print("ERROR: Radio initialization failed: ");
        Serial.println(status);
        halt();
    }
    

    pinMode(BOARD_PWR, OUTPUT);
    digitalWrite(BOARD_PWR, HIGH);

    delay(200);
    Wire.begin(BOARD_SDA, BOARD_SCL);
    Wire.requestFrom(LILYGO_KB_ADDR, 1);
    if (Wire.read() == -1) {
        Serial.println("Keyboard not found");
        while (1) {
            delay(1000);
        }
    }

    display_setup();
}

void loop() {
    char key_value = 0;

    Wire.requestFrom(LILYGO_KB_ADDR, 1);
    if (Wire.available()) {
        key_value = Wire.read();
        if (key_value != 0) {
            message_buffer[message_buffer_cursor++ % sizeof(message_buffer)] = key_value;
            
        }
    }

    display_loop(message_buffer);
}
