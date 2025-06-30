#pragma once

#include <stddef.h>

class BlinkPattern {
  bool _initial_state = false; // state of the led at the beginning of the periodic pattern
  bool _state = false; // current state of the led
  unsigned long _pattern[8]; // times at which the led state changes [t0, t1, t2, ...]
  size_t _pattern_size;
  size_t _pattern_idx = 0;

  int _next_change = 0;

public:

  BlinkPattern(bool initial_state, const unsigned long* pattern, size_t size);

  void begin(unsigned long t);
  bool loop(unsigned long t);
  bool state() const;
};
