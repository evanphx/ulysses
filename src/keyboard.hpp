#ifndef KEYBOARD_H
#define KEYBOARD_H

#include "io.hpp"
#include "cpu.hpp"
#include "buffer.hpp"

enum KeyboardKeyType {
  none=0, reg,
  enter, ctrl, lshift, rshift,
  leftalt, caps, fc,
  home, up, page_up, left, right, end, down,
  page_down
};

class Thread;

class Keyboard {
  static const int cDefaultBufferSize = cpu::cPageSize;
  static KeyboardKeyType mapping[128];
  static const char us_map[0x3a][2];
  static const char dv_map[0x3a][2];

  IOPort io_;
  IOPort status_;

  int shifted_;

  const char (*map_)[2];

  Thread* thread_;

  Buffer buffer_;

public:

  Buffer& buffer() {
    return buffer_;
  }

  void schedule_thread();
  void in_thread();
  void pressed();
  void init();

  void put(char keycode);
};

extern Keyboard keyboard;

#endif

