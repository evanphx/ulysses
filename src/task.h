// 
// task.h - Defines the structures and prototypes needed to multitask.
//          Written for JamesM's kernel development tutorials.
//

#ifndef TASK_H
#define TASK_H

#include "common.h"
#include "paging.h"

#define KERNEL_STACK_SIZE 2048       // Use a 2kb kernel stack.

// This structure defines a 'task' - a process.
struct Task {
  int id;              // Process ID.
  u32 esp, ebp;       // Stack and base pointers.
  u32 eip;            // Instruction pointer.
  page_directory* directory; // Page directory.
  u32int kernel_stack;   // Kernel stack location.
  Task *next;     // The next task in a linked list.
};

struct Scheduler {
  volatile Task* current_task;
  volatile Task* ready_queue;

  u32 next_pid;
  void init();

  void switch_task();
  int fork();

  int getpid();

  void switch_to_user_mode();
};

extern Scheduler scheduler;

#endif
