#ifndef SCHEDULER_HPP
#define SCHEDULER_HPP

#include "thread.hpp"
#include "process.hpp"
#include "constants.hpp"
#include "common.hpp"

class Scheduler {
public:
  Thread* current;

  Thread::RunList waiting_queue;

private:
  Process* processes_[constants::cMaxProcesses];
  Process::CleanupList cleanup_;
  Thread::RunList ready_queue_;
  Thread* idle_thread_;

public:

  void init();

  int new_pid();

  void make_ready(Thread* task) {
    task->state = Thread::eReady;
    ready_queue_.prepend(task);
  }

  void make_wait(Thread* task) {
    task->state = Thread::eWaiting;
    waiting_queue.prepend(task);
  }

  Process* process() {
    return current->process();
  }

  void remove_from_ready(Thread* thr) {
    ready_queue_.unlink(thr);
  }

  void remove_from_waiting(Thread* thr) {
    waiting_queue.unlink(thr);
  }

  void cleanup();
  void switch_task();
  int fork();

  int spawn_thread(void (*func)(void));
  int spawn_init(void (*func)(void));

  void exit(int code);
  void sleep(int secs);

  void io_wait();

  int getpid();
  int wait_any(int* status);

  void on_tick();

  void* heap_start();
  void* change_heap(int bytes);

  Process* find_process(int pid);
};

extern Scheduler scheduler;

#endif
