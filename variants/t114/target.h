#pragma once

#include <helpers/nrf52/T114Board.h>
#include <helpers/CustomSX1262Wrapper.h>

extern T114Board board;
extern RADIO_CLASS radio;

bool radio_init();
