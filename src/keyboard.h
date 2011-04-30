#ifndef KEYBOARD_H
#define KEYBOARD_H

#include "io.h"

struct Keyboard {
  IOPort io;

  void pressed();
  void init();
};

extern Keyboard keyboard;

#endif

