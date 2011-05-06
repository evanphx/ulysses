// monitor.h -- Defines the interface for monitor.h
//              From JamesM's kernel development tutorials.

#ifndef MONITOR_H
#define MONITOR_H

#include <stdarg.h>

#include "common.hpp"
#include "io.hpp"

struct Console {
  u16* video_memory;
  IOPort ctrl;
  IOPort data;

  u8 x;
  u8 y;

  void move_cursor();
  void scroll();
  void put(char c);
  void setup();
  void clear();
  void write(const char* c);
  void puts(const char* c);
  void write_hex(int u);
  void write_hex_np(int n);
  void write_hex_byte(u8 byte);
  void write_dec(int n);
  void write_dec_ll(u64 n);

  void printf(const char* fmt, ...);
  void vprintf(const char* fmt, va_list ap);

};

extern Console console;

extern "C" {
  void kputs(const char *c);
  void kprintf(const char* fmt, ...);
}

#endif // MONITOR_H
