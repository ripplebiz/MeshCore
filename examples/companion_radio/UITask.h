#pragma once

#include <MeshCore.h>
#include <helpers/ui/DisplayDriver.h>
#include <helpers/SensorManager.h>
#include <stddef.h>

#ifdef PIN_BUZZER
  #include <helpers/ui/buzzer.h>
#endif

#include "NodePrefs.h"
#include "Button.h"

 enum class UIEventType
{
    none,
    contactMessage,
    channelMessage,
    roomMessage,
    newContactMessage,
    ack
};

class UITask {
  DisplayDriver* _display;
  mesh::MainBoard* _board;
  SensorManager* _sensors;
#ifdef PIN_BUZZER
  genericBuzzer buzzer;
#endif
  unsigned long _next_refresh, _auto_off;
  bool _connected;
  NodePrefs* _node_prefs;
  char _version_info[32];
  char _origin[62];
  char _msg[80];
  char _alert[80];
  int _msgcount;
  bool _need_refresh = true;
  bool _displayWasOn = false;  // Track display state before button press
  unsigned long ui_started_at;

  // Button handlers
#ifdef PIN_USER_BTN
  Button* _userButton = nullptr;
#endif
#ifdef PIN_USER_BTN_ANA
  Button* _userButtonAnalog = nullptr;
#endif

  void renderCurrScreen();
  void userLedHandler();
  void renderBatteryIndicator(uint16_t batteryMilliVolts);
  
  // Button action handlers
  void handleButtonAnyPress();
  void handleButtonShortPress();
  void handleButtonDoublePress();
  void handleButtonTriplePress();
  void handleButtonQuadruplePress();
  void handleButtonLongPress();

 
public:

  UITask(mesh::MainBoard* board) : _board(board), _display(NULL), _sensors(NULL) {
      _next_refresh = 0;
      ui_started_at = 0;
      _connected = false;
  }
  void begin(DisplayDriver* display, SensorManager* sensors, NodePrefs* node_prefs);

  void setHasConnection(bool connected) { _connected = connected; }
  bool hasDisplay() const { return _display != NULL; }
  void clearMsgPreview();
  void msgRead(int msgcount);
  void newMsg(uint8_t path_len, const char* from_name, const char* text, int msgcount);
  void soundBuzzer(UIEventType bet = UIEventType::none);
  void shutdown(bool restart = false);
  void loop();
  
  // Power management support
  uint32_t getNextRefreshTime() const { return _next_refresh; }
};
