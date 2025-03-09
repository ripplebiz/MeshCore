#pragma once

#include <helpers/ui/DisplayDriver.h>

class UITask {
  DisplayDriver* _display;
  unsigned long _next_read, _next_refresh, _auto_off;
  int _prevBtnState;
  const char* _node_name;
  const char* _build_date;

  void renderCurrScreen();
public:
  UITask(DisplayDriver& display) : _display(&display) { _next_read = _next_refresh = 0; }
  void begin(const char* node_name, const char* build_date);

  void loop();
};