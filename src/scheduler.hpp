#ifndef SCHEDULER_HPP
#define SCHEDULER_HPP

#include "thread.hpp"
#include "process.hpp"
#include "constants.hpp"
#include "common.hpp"
#include "spinlock.hpp"

#include "character/console.hpp"

extern "C" void start_new_thread(void (*func)(), Thread* th);

class Scheduler {
public:
  Thread* current;

  Thread::RunList waiting_queue;

private:
  Process* processes_[constants::cMaxProcesses];
  Process::CleanupList cleanup_;
  Thread::RunList ready_queue_;

  sys::ExternalList<Thread*> hiprio_threads_;

  Thread* idle_thread_;

  console_driver::ConsoleDevice* console_;

public:

  void init();

  int new_pid();

  void make_ready(Thread* thread) {
    thread->state_ = Thread::eReady;
    ready_queue_.prepend(thread);
  }

  void make_wait(Thread* thread) {
    thread->state_ = Thread::eWaiting;
    waiting_queue.prepend(thread);
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

  PosixSession& session() {
    return process()->session();
  }

  int euid() {
    return process()->session().euid();
  }

  void set_console(console_driver::ConsoleDevice* dev) {
    console_ = dev;
  }

  void cleanup();
  bool switch_thread();
  int fork();

  void start_new_thread(void (*func)(), Thread* th);
  Thread* spawn_thread(void (*func)(void));
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

  int process_group(int pid=0);
  int process_id();

  void process_keyboard();

  void schedule_hiprio(Thread* thr);
};

extern Scheduler scheduler;

#endif
