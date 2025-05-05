#pragma once

#include <MeshCore.h>
#include <helpers/ui/DisplayDriver.h>
#include <stddef.h>

#include "NodePrefs.h"

class UITask {
  DisplayDriver* _display;
  mesh::MainBoard* _board;
  unsigned long _next_refresh, _auto_off;
  bool _connected;
  uint32_t _pin_code;
  NodePrefs* _node_prefs;
  char _version_info[32];
  char _origin[62];
  char _msg[80];
  int _msgcount;
  bool _need_refresh = true;

  void renderCurrScreen();
  void buttonHandler();
  void userLedHandler();

public:

  UITask(mesh::MainBoard* board) : _board(board), _display(NULL) {
      _next_refresh = 0; 
      _connected = false;
  }
  void begin(DisplayDriver* display, NodePrefs* node_prefs, const char* build_date, const char* firmware_version, uint32_t pin_code);

  void setHasConnection(bool connected) { _connected = connected; }
  bool hasDisplay() const { return _display != NULL; }
  void clearMsgPreview();
  void msgRead(int msgcount);
  void newMsg(uint8_t path_len, const char* from_name, const char* text, int msgcount);
  void loop();
};
