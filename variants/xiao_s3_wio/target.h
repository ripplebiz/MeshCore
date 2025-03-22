#pragma once

#include <helpers/ESP32Board.h>
#include <helpers/CustomSX1262Wrapper.h>

extern ESP32Board board;
extern RADIO_CLASS radio;

bool radio_init();
