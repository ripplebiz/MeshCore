#pragma once

#include <helpers/StationG2Board.h>
#include <helpers/CustomSX1262Wrapper.h>

extern StationG2Board board;
extern RADIO_CLASS radio;

bool radio_init();
