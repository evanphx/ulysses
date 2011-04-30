// monitor.h -- Defines the interface for monitor.h
//              From JamesM's kernel development tutorials.

#ifndef MONITOR_H
#define MONITOR_H

#include "common.h"

struct Console {
  u16* video_memory;

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

};

extern Console console;

extern "C" {

void monitor_setup();

// Write a single character out to the screen.
void monitor_put(char c);

// Clear the screen to all black.
void monitor_clear();

// Output a null-terminated ASCII string to the monitor.
void monitor_write(const char *c);

void kputs(const char *c);

// Output a hex value to the monitor.
void monitor_write_hex(u32int n);
void monitor_write_hex_np(u32int n);
void monitor_write_hex_byte(u8int n);

// Output a decimal value to the monitor.
void monitor_write_dec(u32int n);
void monitor_write_dec_ll(u64 n);

void kprintf(const char* fmt, ...);

}

#endif // MONITOR_H
