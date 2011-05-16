#include "block.hpp"
#include "kheap.hpp"
#include "console.hpp"
#include "fs/devfs.hpp"

namespace block {
  Registry registry;

  void Registry::init() {
    used_ = 0;

    for(int i = 0; i < max_devices; i++) {
      devices_[i] = 0;
    }
  }

  int Registry::add(Device* dev) {
    ASSERT(used_ < max_devices);
    dev->id_ = ++used_;

    devices_[dev->id_] = dev;

    devfs::main.add_block_device(dev, dev->name());

    return dev->id_;
  }

  void Registry::print() {
    for(int i = 1; i < max_devices; i++) {
      if(Device* dev = devices_[i]) {
        console.printf("%4d: %s\n", i, dev->name());
      }
    }
  }

  u8* Device::read(u32 block, u8 count) {
    u32 size = count * BlockSize;
    u8* buffer = (u8*)kmalloc(size);

    read_block(block, buffer);
    return buffer;
  }

  u8* Device::find_block(u32 block) {
    u8* buffer = 0;
    if(cache_.fetch(block, &buffer)) {
    } else {
      buffer = (u8*)kmalloc(BlockSize);
      read_block(block, buffer);
      cache_.store(block, buffer);
    }

    return buffer;
  }

  u32 Device::read_bytes(u32 offset, u32 size, u8* out_buffer) {

    u32 cur_block = offset / BlockSize;

    u32 initial_offset = offset % BlockSize;

    u32 left = size;

    // Do the first block by itself to deal with the
    // initial offset here.
    u8* buffer = find_block(cur_block);

    if(left > BlockSize) {
      u32 copy_bytes = BlockSize - initial_offset;
      memcpy(out_buffer, buffer + initial_offset, copy_bytes);
      out_buffer += copy_bytes;
      left -= copy_bytes;
    } else {
      // Satisfied all out of the first block! Woo!
      memcpy(out_buffer, buffer + initial_offset, left);
      return size;
    }

    while(left > 0) {
      cur_block++;

      buffer = find_block(cur_block);

      if(left > BlockSize) {
        memcpy(out_buffer, buffer, BlockSize);
        left -= BlockSize;
        out_buffer += BlockSize;
      } else {
        memcpy(out_buffer, buffer, left);
        left = 0;
      }
    }

    return size;
  }

  void SubDevice::read_block(u32 block, u8* buffer) {
    parent_->read_block(block + offset_, buffer);
  }
}
