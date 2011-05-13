#ifndef IO_H
#define IO_H

#include "common.hpp"

struct IOPort {
  u16 port;

  /*
  explicit IOPort()
    : port(0)
  {}

  explicit IOPort(u16 p)
    : port(p)
  {}
  */

  void outb(u8 value, int offset=0) {
    u16 dest = port + offset;
    asm volatile ("outb %1, %0" : : "dN" (dest), "a" (value));
  }

  // Write a byte out to the specified port.
  void outw(u16 value, int offset=0) {
    u16 dest = port + offset;
    asm volatile ("outw %1, %0" : : "dN" (dest), "a" (value));
  }

  // Write a byte out to the specified port.
  void outl(u32 value, int offset=0) {
    u16 dest = port + offset;
    asm volatile ("outl %1, %0" : : "dN" (dest), "a" (value));
  }

  u8 inb(int offset=0) {
    u16 dest = port + offset;
    u8 ret;
    asm volatile("inb %1, %0" : "=a" (ret) : "dN" (dest));
    return ret;
  }

  u16 inw(int offset=0) {
    u16 dest = port + offset;
    u16 ret;
    asm volatile ("inw %1, %0" : "=a" (ret) : "dN" (dest));
    return ret;
  }

  u32 inl(int offset=0) {
    u16 dest = port + offset;
    u32 ret;
    asm volatile ("inl %1, %0" : "=a" (ret) : "dN" (dest));
    return ret;
  }
};

#endif
