#include "character/console.hpp"
#include "fs/devfs.hpp"
#include "monitor.hpp"
#include "termios.h"
#include "ioctl.h"

namespace console_driver {

  u32 ConsoleDevice::read_bytes(u32 offset, u32 size, u8* buffer) {
    console.printf("request %d bytes from console\n", size);
    memcpy(buffer, (u8*)"hello\n\0", 7);
    return 7;
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
        return 0;
      }
      break;
    default:
      console.printf("unknown ioctl: %x\n", req);
      return -1;
    }
  }

  void init() {
    ConsoleDevice* dev = new(kheap) ConsoleDevice;
    devfs::main.add_char_device(dev, "console");
  }
}
