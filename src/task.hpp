// 
// task.hpp - Defines the structures and prototypes needed to multitask.
//          Written for JamesM's kernel development tutorials.
//

#ifndef TASK_H
#define TASK_H

#include "common.hpp"
#include "paging.hpp"
#include "list.hpp"
#include "fs.hpp"

#define KERNEL_STACK_SIZE 2048       // Use a 2kb kernel stack.

// This structure defines a 'task' - a process.
struct Task {
  struct SavedRegisters {
    u32 eip, esp, ebp;
    u32 edi, esi, ebx;

    SavedRegisters()
      : eip(0), esp(0), ebp(0)
      , edi(0), esi(0), ebx(0)
    {}
  };

  enum State {
    eReady,
    eWaiting,
    eDead
  };

  enum Lists {
    cRun = 0,
    cChild = 1,
    cTotal = 2
  };

  typedef sys::List<Task, cRun> RunList;
  typedef sys::List<Task, cChild> ChildList;
  typedef sys::ExternalList<MemoryMapping> MMapList;

  Task(int pid);

  int id;              // Process ID.
  SavedRegisters regs;
  State state;
  x86::PageDirectory* directory; // Page directory.
  u32 kernel_stack;   // Kernel stack location.

  u32 alarm_at;
  int exit_code;

  sys::ListNode<Task> lists[cTotal];
  MMapList mmaps;

  fs::File* fds_[16];

public:
  Task* next_runnable() {
    return lists[cRun].next;
  }

  int find_fd() {
    for(int i = 0; i < 16; i++) {
      if(fds_[i] == 0) return i;
    }

    return -1;
  }

  void sleep_til(int secs);
  bool alarm_expired();

  void add_mmap(fs::Node* node, u32 offset, u32 size, u32 addr, u32 mem_size, int flags);
  MemoryMapping* find_mapping(u32 addr);

  int open_file(const char* name, int mode);
  fs::File* get_file(int fd);
};

struct Scheduler {
  u32 next_pid;
  Task* current;

  Task::RunList ready_queue;
  Task::RunList cleanup_queue;
  Task::RunList waiting_queue;

  void init();

  void make_ready(Task* task) {
    task->state = Task::eReady;
    ready_queue.prepend(task);
  }

  void make_wait(Task* task) {
    task->state = Task::eWaiting;
    waiting_queue.prepend(task);
  }

  void cleanup();
  void switch_task();
  int fork();

  void exit(int code);
  void sleep(int secs);

  int getpid();
  int wait_any(int* status);

  void switch_to_user_mode();
  void on_tick();
};

extern Scheduler scheduler;

#endif
