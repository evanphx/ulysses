#ifndef BUFFER_HPP
#define BUFFER_HPP

#include "common.hpp"

class Buffer;

class Region {
  u8* start_;
  u8* end_;

public:
  Region(u8* start, u8* end)
    : start_(start)
    , end_(end)
  {}

  Region()
    : start_(0)
    , end_(0)
  {}

  bool empty_p() {
    return end_ <= start_;
  }

  int size() {
    return end_ - start_;
  }

  int drain_into(u8* buffer, int size);

  friend class Buffer;
};

class Buffer {
  int used_;
  int size_;

  u8* data_;

public:
  Buffer()
    : used_(0)
    , size_(0)
    , data_(0)
  {}

  int used() {
    return used_;
  }

  bool has_data_p() {
    return used_ > 0;
  }

  void allocate(int size);

  bool copy(Buffer& other);

  void clear() {
    used_ = 0;
  }

  bool put(u8 byte) {
    if(used_ == size_) return false;
    data_[used_++] = byte;
    return true;
  }

  void all_of(Region& reg) {
    reg.start_ = data_;
    reg.end_   = data_ + used_;
  }

  bool find_newline(Region& reg);

  template <typename T>
    void each_byte(T func) {
      u8* end = data_ + used_;
      u8* pos = data_;

      while(pos < end) func(*pos++);
    }

};

#endif
