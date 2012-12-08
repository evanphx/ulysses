#ifndef CHARACTER_CONSOLE_HPP
#define CHARACTER_CONSOLE_HPP

#include "character.hpp"
#include "fs.hpp"

namespace console_driver {
  class Node : public fs::Node {
    u32 write(u32 offset, u32 size, u8* buffer) {
      return 0;
    }
  };

  class ConsoleDevice : public character::Device {
    u32 read_bytes(u32 offset, u32 size, u8* buffer);
    u32 write_bytes(u32 offset, u32 size, u8* buffer);
    int ioctl(unsigned long req, va_list args);
  };

  void init();
}

#endif
