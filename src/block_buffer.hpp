#ifndef BLOCK_BUFFER_HPP
#define BLOCK_BUFFER_HPP

#include "block_region.hpp"
#include "spinlock.hpp"

class Thread;

namespace block {
  class Device;

  class Buffer {
    enum States {
      eUnknown = 0,
      eFree = 0x1,
      eFull = 0x2,
      eRequested = 0x4
    };

  private:
    u16 size_;
    u16 state_;
    u8* data_;

    Device* device_;

    RegionRange range_;
    Thread* waiting_task_;

    SpinLock lock_;

  public:
    Buffer(u16 size, u8* data, Device* dev=0)
      : size_(size)
      , state_(eFree)
      , data_(data)
      , device_(dev)
      , range_(0,0)
      , waiting_task_(0)
    {}

    u16 size() {
      return size_;
    }

    u32 byte_size() {
      return size_;
    }

    u8* data() {
      return data_;
    }

    template <typename T>
      T as() {
        return (T)data_;
      }

    template <typename T>
      T index(u32 i) {
        return ((T*)data_)[i];
      }

    bool free_p() {
      return (state_ & eFree) == eFree;
    }

    bool full_p() {
      return (state_ & eFull) == eFull;
    }

    bool requested_p() {
      return (state_ & eRequested) == eRequested;
    }

    void set_full();

    RegionRange& range() {
      return range_;
    }

    void set_range(RegionRange& range) {
      range_ = range;
    }

    void set_waiting_task(Thread* task) {
      waiting_task_ = task;
    }

    void fill();
    void busy_wait();
    void wait();

    static Buffer* for_size(Device* dev, u32 size);
  };
}

#endif
