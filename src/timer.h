// timer.h -- Defines the interface for all PIT-related functions.
//            Written for JamesM's kernel development tutorials.

#ifndef TIMER_H
#define TIMER_H

#include "common.h"

void init_timer(u32int frequency);

#define NSEC_PER_SEC 1000000000
#define SLICE_HZ 100
#define SLICE_US 10000
#endif
