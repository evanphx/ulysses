#ifndef THREAD_H
#define THREAD_H

#include "common.hpp"
#include "paging.hpp"
#include "list.hpp"
#include "fs.hpp"

#define KERNEL_STACK_SIZE 4096       // Use a 4kb (one page) kernel stack.

namespace ipc {
  class Channel;
}

class Process;

class Thread {
public:
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
    eRunning,

    eWaiting,
    eIOWait,

    eDieing,
    eDead
  };

  enum Lists {
    cRun = 0,
    cChild = 1,
    cProcess = 2,
    cTotal = 3
  };

  typedef sys::List<Thread, cRun> RunList;
  typedef sys::List<Thread, cChild> ChildList;
  typedef sys::List<Thread, cProcess> ProcessList;

private:
  Process* process_;
  int id_;

public:
  Thread(Process* process, int id);

public:
  SavedRegisters regs;
  State state;
  x86::PageDirectory* directory;
  u32 kernel_stack;

  u32 alarm_at;

  sys::ListNode<Thread> lists[cTotal];

public:
  int id() {
    return id_;
  }

  bool dead() {
    return state == eDead;
  }

  Process* process() {
    return process_;
  }

  Thread* next_runnable() {
    return lists[cRun].next;
  }

  Thread* next_in_process() {
    return lists[cProcess].next;
  }

  void sleep_til(int secs);
  bool alarm_expired();

  void die();
};

#endif
