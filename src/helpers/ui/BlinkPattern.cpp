#include "BlinkPattern.h"

BlinkPattern::BlinkPattern(bool initial_state, const unsigned long* pattern, size_t size)
  :_initial_state(initial_state)
{
  _pattern_size = size > 8 ? 8 : size;

  for (size_t i = 0; i < _pattern_size; ++i) {
    _pattern[i] = pattern[i];
  }
}

void BlinkPattern::begin(unsigned long t) {
  _pattern_idx = _pattern_size - 1;
  _state = !_initial_state;
  _next_change = t;
}

bool BlinkPattern::loop(unsigned long t) {
  if (t >= _next_change) {
    size_t next_idx = (_pattern_idx + 1) % _pattern_size;
    _next_change += _pattern[next_idx];

    if (next_idx != 0) {
      _next_change -= _pattern[_pattern_idx];
    }

    _state = !_state;
    _pattern_idx = next_idx;

    return true;
  }

  return false;
}

bool BlinkPattern::state() const {
  return _state;
}
