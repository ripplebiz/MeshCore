#pragma once

#include <helpers/LilyGoTLoraBoard.h>
#include <helpers/CustomSX1276Wrapper.h>

extern LilyGoTLoraBoard board;
extern RADIO_CLASS radio;

bool radio_init();
