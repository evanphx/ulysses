#ifndef CHARACTER_HPP
#define CHARACTER_HPP

#include "common.hpp"
#include <stdarg.h>

namespace character {
  class Device {
  public:
    virtual u32 read_bytes(u32 offset, u32 size, u8* buffer) = 0;
    virtual u32 write_bytes(u32 offset, u32 size, u8* buffer) = 0;
    virtual int ioctl(unsigned long req, va_list args) = 0;
  };
}

#endif
