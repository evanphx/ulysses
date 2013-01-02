#ifndef SPINLOCK_HPP
#define SPINLOCK_HPP

#include "percpu.hpp"
#include "cpu.hpp"

class Thread;

class SpinLock {
  Thread* locker_;
  int recursive_;
  bool enable_interrupts_;

  const char* file_;
  int line_;

public:

  SpinLock()
    : locker_(0)
    , recursive_(0)
    , enable_interrupts_(false)
  {}

  void lock(const char* f=0, int l=-1) {
    Thread* cur = PerCPU::thread();

    enable_interrupts_ = cpu::interrupts_enabled_p();

    if(enable_interrupts_) cpu::disable_interrupts();

    if(!__sync_bool_compare_and_swap(&locker_, 0, cur)) {
      if(locker_ == cur) {
        recursive_++;
      } else {
        while(!__sync_bool_compare_and_swap(&locker_, 0, cur));
      }
    }

    file_ = f;
    line_ = l;
  }

  void unlock() {
    Thread* cur = PerCPU::thread();

    ASSERT(locker_ == cur);

    file_ = 0;
    line_ = -1;

    if(recursive_ > 0) {
      recursive_--;
    } else {
      // Use a sync primitive because it includes the required memory
      // barrier to make sure other CPUs see the change in value_
      // properly.
      ASSERT(__sync_bool_compare_and_swap(&locker_, cur, 0));
    }

    if(enable_interrupts_) cpu::enable_interrupts();
  }

  void force_unlock() {
    ASSERT(recursive_ == 0);
    file_ = 0;
    line_ = -1;
    locker_ = 0;
    __sync_synchronize();
  }
};

#endif
