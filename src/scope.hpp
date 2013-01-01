#ifndef SCOPE_HPP
#define SCOPE_HPP

#include "cpu.hpp"

class SuspendInterrupts {
  int cond_;

public:
  SuspendInterrupts() {
    cond_ = cpu::disable_interrupts();
  }

  ~SuspendInterrupts() {
    cpu::restore_interrupts(cond_);
  }
};

template <class T>
class SyncronizeWrapper {
  bool cond_;
  T& lock_;

public:
  SyncronizeWrapper(T& lock, const char* f = 0, int l = 0)
    : cond_(true)
    , lock_(lock)
  {
    lock.lock(f,l);
  }

  ~SyncronizeWrapper() {
    lock_.unlock();
  }

  bool continue_p() {
    return cond_;
  }

  void done() {
    cond_ = false;
  }
};

#define synchronized(lock) for(SyncronizeWrapper<SpinLock> __sw(lock, __FILE__, __LINE__); __sw.continue_p(); __sw.done())

#endif
