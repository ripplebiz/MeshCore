#include <Arduino.h>
#include <SPI.h>
#include <TFT_eSPI.h>
#include "utilities.h"

#define USER_SETUP_ID 210

typedef struct {
    uint8_t cmd;
    uint8_t data[14];
    uint8_t len;
} lcd_cmd_t;

lcd_cmd_t lcd_st7789v[] = {
    {0x01, {0}, 0 | 0x80},
    {0x11, {0}, 0 | 0x80},
    {0x3A, {0X05}, 1},
    {0x36, {0x55}, 1},
    {0xB2, {0x0C, 0x0C, 0X00, 0X33, 0X33}, 5},
    {0xB7, {0X75}, 1},
    {0xBB, {0X1A}, 1},
    {0xC0, {0X2C}, 1},
    {0xC2, {0X01}, 1},
    {0xC3, {0X13}, 1},
    {0xC4, {0X20}, 1},
    {0xC6, {0X0F}, 1},
    {0xD0, {0XA4, 0XA1}, 2},
    {0xD6, {0XA1}, 1},
    {0xE0, {0XD0, 0X0D, 0X14, 0X0D, 0X0D, 0X09, 0X38, 0X44, 0X4E, 0X3A, 0X17, 0X18, 0X2F, 0X30}, 14},
    {0xE1, {0XD0, 0X09, 0X0F, 0X08, 0X07, 0X14, 0X37, 0X44, 0X4D, 0X38, 0X15, 0X16, 0X2C, 0X3E}, 14},
    {0x21, {0}, 0}, //invertDisplay
    {0x29, {0}, 0},
    {0x2C, {0}, 0},
};

TFT_eSPI  tft;

// LilyGo  T-Deck  control backlight chip has 16 levels of adjustment range
// The adjustable range is 0~15, 0 is the minimum brightness, 16 is the maximum brightness
void setBrightness(uint8_t value)
{
    static uint8_t level = 0;
    static uint8_t steps = 16;
    if (value == 0) {
        digitalWrite(BOARD_BL_PIN, 0);
        delay(3);
        level = 0;
        return;
    }
    if (level == 0) {
        digitalWrite(BOARD_BL_PIN, 1);
        level = steps;
        delayMicroseconds(30);
    }
    int from = steps - level;
    int to = steps - value;
    int num = (steps + to - from) % steps;
    for (int i = 0; i < num; i++) {
        digitalWrite(BOARD_BL_PIN, 0);
        digitalWrite(BOARD_BL_PIN, 1);
    }
    level = value;
}


void setup()
{
    Serial.begin(115200);

    Serial.println("T-DECK factory");

    //! The board peripheral power control pin needs to be set to HIGH when using the peripheral
    pinMode(BOARD_POWERON, OUTPUT);
    digitalWrite(BOARD_POWERON, HIGH);

    //! Set CS on all SPI buses to high level during initialization
    pinMode(BOARD_SDCARD_CS, OUTPUT);
    pinMode(RADIO_CS_PIN, OUTPUT);
    pinMode(BOARD_TFT_CS, OUTPUT);

    digitalWrite(BOARD_SDCARD_CS, HIGH);
    digitalWrite(RADIO_CS_PIN, HIGH);
    digitalWrite(BOARD_TFT_CS, HIGH);

    pinMode(BOARD_SPI_MISO, INPUT_PULLUP);
    SPI.begin(BOARD_SPI_SCK, BOARD_SPI_MISO, BOARD_SPI_MOSI); //SD

    Serial.print("Init display id:");
    Serial.println(USER_SETUP_ID);

    tft.begin();

    /**
     * * T-Deck-Plus and T-Deck display panels are different.
     * * This initialization is used to override the initialization parameters.
     * * It is used when the display is abnormal after the TFT_eSPI update. */
#if 0
    for (uint8_t i = 0; i < (sizeof(lcd_st7789v) / sizeof(lcd_cmd_t)); i++) {
        tft.writecommand(lcd_st7789v[i].cmd);
        for (int j = 0; j < (lcd_st7789v[i].len & 0x7f); j++) {
            tft.writedata(lcd_st7789v[i].data[j]);
        }

        if (lcd_st7789v[i].len & 0x80) {
            delay(120);
        }
    }
#endif

    tft.setRotation( 1 );
    tft.fillScreen(TFT_BLACK);

    tft.setTextDatum(MC_DATUM);
    tft.setFreeFont(&FreeSansOblique12pt7b);
    tft.drawString("Hello World", TFT_WIDTH / 2, TFT_HEIGHT / 2);


    pinMode(BOARD_BL_PIN, OUTPUT);
    for (int i = 0; i <= 16; ++i) {
        setBrightness(i);
        delay(30);
    }
}

void loop()
{
    tft.setTextColor(random(0xFFFF));
    tft.fillScreen(TFT_BLACK);
    tft.drawString("Hello World", random(5, 320), random(5, 240));
    delay(5000);
}
