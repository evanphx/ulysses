#include "character/console.hpp"
#include "fs/devfs.hpp"
#include "monitor.hpp"
#include "termios.h"
#include "ioctl.h"
#include "scheduler.hpp"
#include "buffer.hpp"

namespace console_driver {

  void ConsoleDevice::inject_bytes(Buffer& buffer) {
    if(tty_.echo_p()) {
      buffer.each_byte([](u8 c) { console.put(c); });
    }

    ASSERT(buffer.copy(raw_buffer_));

    if(tty_.canonical_p()) {
      if(raw_buffer_.find_newline(read_region_)) {
        read_wait_.wake();
      }
    } else {
      raw_buffer_.all_of(read_region_);
      read_wait_.wake();
    }
  }

  u32 ConsoleDevice::read_bytes(u32 offset, u32 size, u8* buffer) {
    while(read_region_.empty_p()) {
      read_wait_.wait();
    }

    return read_region_.drain_into(buffer, size);
  }

  u32 ConsoleDevice::write_bytes(u32 offset, u32 size, u8* buffer) {
    for(u32 i = 0; i < size; i++) {
      console.put(buffer[i]);
    }
    return size;
  }

  int ConsoleDevice::ioctl(unsigned long req, va_list args) {
    switch(req) {
    case TCGETS:
      {
        termios* info = va_arg(args, termios*);
        memset((u8*)info, 0, sizeof(termios));
        if(tty_.echo_p()) {
          info->c_lflag |= ECHO;
        }

        if(tty_.canonical_p()) {
          info->c_iflag |= ICANON;
        }

        info->c_oflag |= OPOST;

        return 0;
      }
      break;
    case TCSETS:
      {
        termios* info = va_arg(args, termios*);

        if(info->c_lflag & ECHO) {
          tty_.set_echo(true);
        } else {
          tty_.set_echo(false);
        }

        if(info->c_iflag & ICANON) {
          tty_.set_canonical(true);
        } else {
          tty_.set_canonical(false);
        }

        return 0;
      }

    default:
      console.printf("unknown ioctl: %x\n", req);
      return -1;
    }
  }

  void ConsoleDevice::open() {
    raw_buffer_.allocate(cpu::cPageSize);
    scheduler.session().set_tty(&tty_);
  }

  void init() {
    ConsoleDevice* dev = new(kheap) ConsoleDevice;
    devfs::main.add_char_device(dev, "console");

    scheduler.set_console(dev);
  }
}
