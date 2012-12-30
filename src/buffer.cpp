#include "buffer.hpp"
#include "kheap.hpp"
#include "algo.hpp"

#include "console.hpp"

void Buffer::allocate(int size) {
  size_ = size;
  data_ = (u8*)kmalloc(size);
}

bool Buffer::copy(Buffer& other) {
  u8* target = other.data_ + other.used_;
  int avail = other.size_ - other.used_;

  if(used_ > avail) return false;

  memcpy(target, data_, used_);
  other.used_ += used_;

  return true;
}

bool Buffer::find_newline(Region& reg) {
  u8* pos = data_;
  u8* end = pos + used_;

  while(pos < end) {
    if(*pos == '\n') {
      reg.start_ = data_;
      reg.end_ = pos + 1;

      return true;
    }

    pos++;
  }

  return false;
}

int Region::drain_into(u8* buffer, int size) {
  int count = algo::min(end_ - start_, size);

  memcpy(buffer, start_, count);
  start_ += count;

  return count;
}
