#ifndef CHARACTER_CONSOLE_HPP
#define CHARACTER_CONSOLE_HPP

#include "character.hpp"
#include "fs.hpp"
#include "tty.hpp"
#include "buffer.hpp"
#include "wait_queue.hpp"

namespace console_driver {
  class Node : public fs::Node {
    u32 write(u32 offset, u32 size, u8* buffer) {
      return 0;
    }
  };

  class ConsoleDevice : public character::Device {
    TTY tty_;
    Buffer raw_buffer_;
    Region read_region_;
    WaitQueue read_wait_;

  public:
    u32 read_bytes(u32 offset, u32 size, u8* buffer);
    u32 write_bytes(u32 offset, u32 size, u8* buffer);
    int ioctl(unsigned long req, va_list args);
    void open();

    void inject_bytes(Buffer& buffer);
  };

  void init();
}

#endif
