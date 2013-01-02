#ifndef STATS_HPP
#define STATS_HPP

#include "atomic.hpp"

struct Stats {
  AtomicInt<int> fast_io;
  AtomicInt<int> slow_io;
};

extern Stats stats;

#endif
