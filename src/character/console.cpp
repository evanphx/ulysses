#include "character/console.hpp"
#include "fs/devfs.hpp"
#include "monitor.hpp"

namespace console_driver {

  u32 ConsoleDevice::read_bytes(u32 offset, u32 size, u8* buffer) {
    return 0;
  }

  u32 ConsoleDevice::write_bytes(u32 offset, u32 size, u8* buffer) {
    for(u32 i = 0; i < size; i++) {
      console.put(buffer[i]);
    }
    return size;
  }

  void init() {
    ConsoleDevice* dev = new(kheap) ConsoleDevice;
    devfs::main.add_char_device(dev, "console");
  }
}
