#ifndef KEYBOARD_H
#define KEYBOARD_H

#include "io.hpp"

enum KeyboardKeyType {
  none=0, reg,
  enter, ctrl, lshift, rshift,
  leftalt, caps, fc,
  home, up, page_up, left, right, end, down,
  page_down
};

struct Keyboard {
  IOPort io;
  IOPort status;

  int shifted;

  static KeyboardKeyType mapping[128];
  static const char us_map[0x3a][2];
  static const char dv_map[0x3a][2];

  const char (*map)[2];

  void pressed();
  void init();
};

extern Keyboard keyboard;

#endif

