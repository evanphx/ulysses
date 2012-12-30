#ifndef WAIT_QUEUE_HPP
#define WAIT_QUEUE_HPP

#include "list.hpp"

class Thread;

class WaitQueue {
  sys::ExternalList<Thread*> threads_;

public:

  bool wake();
  void wait();
};

#endif
