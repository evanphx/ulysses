// timer.h -- Defines the interface for all PIT-related functions.
//            Written for JamesM's kernel development tutorials.

#ifndef TIMER_H
#define TIMER_H

#include "common.h"

#define NSEC_PER_SEC 1000000000
#define SLICE_HZ 100
#define SLICE_US 10000

struct Timer {
  u32 ticks;

  u32 secs_to_ticks(int secs) {
    return secs * SLICE_HZ;
  }

  void init(u32 frequency);
};

extern Timer timer;

#endif
