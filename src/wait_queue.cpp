#include "wait_queue.hpp"
#include "thread.hpp"
#include "scheduler.hpp"

#include "console.hpp"

bool WaitQueue::wake() {
  Thread* th = 0;
  if(!threads_.shift(&th)) return false;

  scheduler.make_ready(th);

  return true;
}

void WaitQueue::wait() {
  auto token = scheduler.start_io();

  threads_.append(scheduler.current());

  scheduler.io_wait(token);
}
