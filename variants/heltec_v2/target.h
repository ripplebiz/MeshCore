#pragma once

#include <helpers/HeltecV2Board.h>
#include <helpers/CustomSX1276Wrapper.h>

extern HeltecV2Board board;
extern RADIO_CLASS radio;

bool radio_init();
