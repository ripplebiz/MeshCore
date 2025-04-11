#include "UITask.h"
#include <Arduino.h>
#include <helpers/TxtDataHelpers.h>

#define AUTO_OFF_MILLIS   15000   // 15 seconds

#ifndef USER_BTN_PRESSED
#define USER_BTN_PRESSED LOW
#endif

// 'meshcore', 128x13px
static const uint8_t meshcore_logo [] PROGMEM = {
    0x3c, 0x01, 0xe3, 0xff, 0xc7, 0xff, 0x8f, 0x03, 0x87, 0xfe, 0x1f, 0xfe, 0x1f, 0xfe, 0x1f, 0xfe, 
    0x3c, 0x03, 0xe3, 0xff, 0xc7, 0xff, 0x8e, 0x03, 0x8f, 0xfe, 0x3f, 0xfe, 0x1f, 0xff, 0x1f, 0xfe, 
    0x3e, 0x03, 0xc3, 0xff, 0x8f, 0xff, 0x0e, 0x07, 0x8f, 0xfe, 0x7f, 0xfe, 0x1f, 0xff, 0x1f, 0xfc, 
    0x3e, 0x07, 0xc7, 0x80, 0x0e, 0x00, 0x0e, 0x07, 0x9e, 0x00, 0x78, 0x0e, 0x3c, 0x0f, 0x1c, 0x00, 
    0x3e, 0x0f, 0xc7, 0x80, 0x1e, 0x00, 0x0e, 0x07, 0x1e, 0x00, 0x70, 0x0e, 0x38, 0x0f, 0x3c, 0x00, 
    0x7f, 0x0f, 0xc7, 0xfe, 0x1f, 0xfc, 0x1f, 0xff, 0x1c, 0x00, 0x70, 0x0e, 0x38, 0x0e, 0x3f, 0xf8, 
    0x7f, 0x1f, 0xc7, 0xfe, 0x0f, 0xff, 0x1f, 0xff, 0x1c, 0x00, 0xf0, 0x0e, 0x38, 0x0e, 0x3f, 0xf8, 
    0x7f, 0x3f, 0xc7, 0xfe, 0x0f, 0xff, 0x1f, 0xff, 0x1c, 0x00, 0xf0, 0x1e, 0x3f, 0xfe, 0x3f, 0xf0, 
    0x77, 0x3b, 0x87, 0x00, 0x00, 0x07, 0x1c, 0x0f, 0x3c, 0x00, 0xe0, 0x1c, 0x7f, 0xfc, 0x38, 0x00, 
    0x77, 0xfb, 0x8f, 0x00, 0x00, 0x07, 0x1c, 0x0f, 0x3c, 0x00, 0xe0, 0x1c, 0x7f, 0xf8, 0x38, 0x00, 
    0x73, 0xf3, 0x8f, 0xff, 0x0f, 0xff, 0x1c, 0x0e, 0x3f, 0xf8, 0xff, 0xfc, 0x70, 0x78, 0x7f, 0xf8, 
    0xe3, 0xe3, 0x8f, 0xff, 0x1f, 0xfe, 0x3c, 0x0e, 0x3f, 0xf8, 0xff, 0xfc, 0x70, 0x3c, 0x7f, 0xf8, 
    0xe3, 0xe3, 0x8f, 0xff, 0x1f, 0xfc, 0x3c, 0x0e, 0x1f, 0xf8, 0xff, 0xf8, 0x70, 0x3c, 0x7f, 0xf8, 
};

void UITask::begin(DisplayDriver* display, const char* node_name, const char* build_date, const char* firmware_version, uint32_t pin_code) {
  _display = display;
  _auto_off = millis() + AUTO_OFF_MILLIS;
  clearMsgPreview();
  _node_name = node_name;
  _pin_code = pin_code;
  if (_display != NULL) {
    _display->turnOn();
  }

  // strip off dash and commit hash by changing dash to null terminator
  // e.g: v1.2.3-abcdef -> v1.2.3
  char *version = strdup(firmware_version);
  char *dash = strchr(version, '-');
  if(dash){
    *dash = 0;
  }

  #ifdef PIN_USER_BTN
    pinMode(PIN_USER_BTN, INPUT);
  #endif

  // v1.2.3 (1 Jan 2025)
  sprintf(_version_info, "%s (%s)", version, build_date);
}

void UITask::msgRead(int msgcount) {
  _msgcount = msgcount;
  if (msgcount == 0) {
    clearMsgPreview();
  }
}

void UITask::clearMsgPreview() {
  _origin[0] = 0;
  _msg[0] = 0;
  _need_refresh = true;
}

void UITask::newMsg(uint8_t path_len, const char* from_name, const char* text, int msgcount) {
  _msgcount = msgcount;

  if (path_len == 0xFF) {
    sprintf(_origin, "(F) %s", from_name);
  } else {
    sprintf(_origin, "(%d) %s", (uint32_t) path_len, from_name);
  }
  StrHelper::strncpy(_msg, text, sizeof(_msg));

  if (_display != NULL) {
    if (!_display->isOn()) _display->turnOn();
    _auto_off = millis() + AUTO_OFF_MILLIS;  // extend the auto-off timer
    _need_refresh = true;
  }
}

void UITask::renderCurrScreen() {
  if (_display == NULL) return;  // assert() ??

  char tmp[80];
  if (_origin[0] && _msg[0]) {
    // render message preview
    _display->setCursor(0, 0);
    _display->setTextSize(1);
    _display->setColor(DisplayDriver::GREEN);
    _display->print(_node_name);

    _display->setCursor(0, 12);
    _display->setColor(DisplayDriver::YELLOW);
    _display->print(_origin);
    _display->setCursor(0, 24);
    _display->setColor(DisplayDriver::LIGHT);
    _display->print(_msg);

    _display->setCursor(100, 9);
    _display->setTextSize(2);
    _display->setColor(DisplayDriver::ORANGE);
    sprintf(tmp, "%d", _msgcount);
    _display->print(tmp);
  } else {
    // render 'home' screen
    _display->setColor(DisplayDriver::BLUE);
    _display->drawXbm(0, 0, meshcore_logo, 128, 13);
    _display->setCursor(0, 20);
    _display->setTextSize(1);

    _display->setColor(DisplayDriver::LIGHT);
    _display->print(_node_name);
    
    _display->setCursor(0, 32);
    _display->print(_version_info);

    if (_connected) {
      //_display->printf("freq : %03.2f sf %d\n", _prefs.freq, _prefs.sf);
      //_display->printf("bw   : %03.2f cr %d\n", _prefs.bw, _prefs.cr);
    } else if (_pin_code != 0) {
      _display->setColor(DisplayDriver::RED);
      _display->setTextSize(2);
      _display->setCursor(0, 43);
      sprintf(tmp, "Pin:%d", _pin_code);
      _display->print(tmp);
    }
  }
  _need_refresh = false;
}

void UITask::userLedHandler() {
#ifdef PIN_STATUS_LED
  static int state = 0;
  static int next_change = 0;
  int cur_time = millis();
  if (cur_time > next_change) {
    if (state == 0) {
      state = 1; // led on, short = unread msg
      if (_msgcount > 0) {
        next_change = cur_time + 500;
      } else {
        next_change = cur_time + 2000;
      }
    } else {
      state = 0;
      if (_board->getBattMilliVolts() > 3800) {
        next_change = cur_time + 2000;
      } else {
        next_change = cur_time + 4000; // 4s blank if bat level low
      }
    }
    digitalWrite(PIN_STATUS_LED, state);
  }
#endif
}

void UITask::buttonHandler() {
#ifdef PIN_USER_BTN
  static int prev_btn_state = !USER_BTN_PRESSED;
  static unsigned long btn_state_change_time = 0;
  static unsigned long next_read = 0;
  int cur_time = millis();
  if (cur_time >= next_read) {
    int btn_state = digitalRead(PIN_USER_BTN);
    if (btn_state != prev_btn_state) {
      if (btn_state == USER_BTN_PRESSED) {  // pressed?
        if (_display != NULL) {
          if (_display->isOn()) {
            clearMsgPreview();
          } else {
            _display->turnOn();
            _need_refresh = true;
          }
          _auto_off = cur_time + AUTO_OFF_MILLIS;   // extend auto-off timer
        }
      } else { // unpressed ? check pressed time ...
        if ((cur_time - btn_state_change_time) > 5000) {
        #ifdef PIN_STATUS_LED
          digitalWrite(PIN_STATUS_LED, LOW);
          delay(10);
        #endif
          _board->powerOff();
        }
      }
      btn_state_change_time = millis();
      prev_btn_state = btn_state;
    }
    next_read = millis() + 100;  // 10 reads per second
  }
#endif
}

void UITask::loop() {
  buttonHandler();
  userLedHandler();

  if (_display != NULL && _display->isOn()) {
    if (millis() >= _next_refresh && _need_refresh) {
      _display->startFrame();
      renderCurrScreen();
      _display->endFrame();

      _next_refresh = millis() + 1000;   // refresh every second
    }
    if (millis() > _auto_off) {
      _display->turnOff();
    }
  }
}
