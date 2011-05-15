#ifndef BLOCK_HPP
#define BLOCK_HPP

#include "common.hpp"
#include "string.hpp"
#include "hash_table.hpp"

namespace block {
  // Must never be less than 512!
  const static u32 BlockSize = 1024;

  class Device;

  class Registry {
  public:
    const static int max_devices = 128;

  private:
    Device* devices_[max_devices];
    int used_;

  public:

    Device* get(int f) {
      if(f < 0 || f > max_devices) return 0;
      return devices_[f];
    }

    void init();
    int add(Device* dev);
    void print();
  };

  extern Registry registry;

  struct BlockCacheOperations {
    static u32 compute_hash(u32 key) {
      // See http://burtleburtle.net/bob/hash/integer.html
      register u32 a = key;
      a = (a+0x7ed55d16) + (a<<12);
      a = (a^0xc761c23c) ^ (a>>19);
      a = (a+0x165667b1) + (a<<5);
      a = (a+0xd3a2646c) ^ (a<<9);
      a = (a+0xfd7046c5) + (a<<3);
      a = (a^0xb55a4f09) ^ (a>>16);
      return a;
    }

    static bool compare_keys(u32 a, u32 b) {
      return a == b;
    }
  };

  typedef sys::HashTable<u32, u8*, BlockCacheOperations> BlockCache;

  class Device {
  protected:
    sys::FixedString<8> name_;
    int id_;

    BlockCache cache_;

  public:
    Device(const char* name)
      : name_(name)
    {}

    int id() {
      return id_;
    }

    const char* name() {
      return name_.c_str();
    }

    u8* find_block(u32 block);

    virtual void read_block(u32 block, u8* buffer) = 0;
    u8* read(u32 block, u8 count);
    u32 read_bytes(u32 byte_offset, u32 byte_size, u8* buffer);

    friend class Registry;
  };

  class SubDevice : public Device {
    u32 offset_;
    u32 size_;

    Device* parent_;

  public:
    SubDevice(const char* name, Device* parent, u32 offset, u32 size)
      : Device(name)
      , offset_(offset)
      , size_(size)
      , parent_(parent)
    {}

    void read_block(u32 block, u8* buffer);
  };

}

#endif
