#ifndef BLOCK_REGION_HPP
#define BLOCK_REGION_HPP

#include "common.hpp"

namespace block {
  const static u32 cRegionSize = 1024;
  const static u32 cSectorPerRegion = 2;

  static inline int byte2region(int count) {
    return count / cRegionSize;
  }

  class Device;

  class RegionRange {
    u32 start_;
    u32 size_;

  public:

    RegionRange(u32 start, u32 size)
      : start_(start)
      , size_(size)
    {}

    u32 start() {
      return start_;
    }

    u32 size() {
      return size_;
    }

    u32 end() {
      return start_ + size_;
    }

    u32 sector() {
      return start_ * cSectorPerRegion;
    }

    u32 num_sectors() {
      return size_ * cSectorPerRegion;
    }

    u32 num_bytes() {
      return size_ * cRegionSize;
    }
  };
}

#endif
