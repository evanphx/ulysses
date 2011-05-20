#ifndef BLOCK_HPP
#define BLOCK_HPP

#include "common.hpp"
#include "string.hpp"
#include "hash_table.hpp"
#include "block_region.hpp"

namespace block {
  // Must never be less than 512!
  const static u32 BlockSize = 1024;

  class Device;
  class Buffer;

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

  struct RegionCacheOperations {
    static u32 compute_hash(RegionRange& key) {
      // See http://burtleburtle.net/bob/hash/integer.html
      register u32 a = key.start() ^ key.size();
      a = (a+0x7ed55d16) + (a<<12);
      a = (a^0xc761c23c) ^ (a>>19);
      a = (a+0x165667b1) + (a<<5);
      a = (a+0xd3a2646c) ^ (a<<9);
      a = (a+0xfd7046c5) + (a<<3);
      a = (a^0xb55a4f09) ^ (a>>16);
      return a;
    }

    static bool compare_keys(RegionRange& a, RegionRange& b) {
      return a.start() == b.start() && a.size() == b.size();
    }
  };

  typedef sys::HashTable<RegionRange, Buffer*,
                         RegionCacheOperations> RegionCache;

  typedef sys::IdentityHash<u32, Buffer*> BufferCache;

  class Buffer;

  class Device {
  protected:
    sys::FixedString<8> name_;
    int id_;

    RegionCache cache_;
    u32 signature_;

  public:
    Device(const char* name)
      : name_(name)
      , signature_(0)
    {}

    int id() {
      return id_;
    }

    const char* name() {
      return name_.c_str();
    }

    Buffer* request(RegionRange range);
    virtual void fulfill(Buffer* buffer) = 0;

    u32 read_bytes(u32 byte_offset, u32 byte_size, u8* buffer);

    void detect_partitions();

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

    void fulfill(Buffer* buffer);
  };

}

#endif
