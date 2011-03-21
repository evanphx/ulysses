// monitor.h -- Defines the interface for monitor.h
//              From JamesM's kernel development tutorials.

#ifndef MONITOR_H
#define MONITOR_H

#include "common.h"

void monitor_setup();

// Write a single character out to the screen.
void monitor_put(char c);

// Clear the screen to all black.
void monitor_clear();

// Output a null-terminated ASCII string to the monitor.
void monitor_write(char *c);

void kputs(char *c);

// Output a hex value to the monitor.
void monitor_write_hex(u32int n);
void monitor_write_hex_np(u32int n);
void monitor_write_hex_byte(u8int n);

// Output a decimal value to the monitor.
void monitor_write_dec(u32int n);
void monitor_write_dec_ll(u64 n);

void kprintf(char* fmt, ...);

#endif // MONITOR_H
