#include "thread.hpp"

#include "paging.hpp"
#include "kheap.hpp"
#include "descriptor_tables.hpp"
#include "cpu.hpp"
#include "console.hpp"
#include "timer.hpp"
#include "fs.hpp"
#include "fs/devfs.hpp"

#include "scheduler.hpp"

Thread::Thread(Process* process, int id)
  : process_(process)
  , id_(id)
  , alarm_at(0)
{}

void Thread::sleep_til(int secs) {
  alarm_at = timer.ticks + timer.secs_to_ticks(secs);
}

bool Thread::alarm_expired() {
  return state_ == Thread::eWaiting && 
         alarm_at != 0 &&
         alarm_at <= timer.ticks;
}

void Thread::die() {
  if(state_ == eWaiting) {
    scheduler.remove_from_waiting(this);
  } else if(state_ == eReady) {
    scheduler.remove_from_ready(this);
  }

  state_ = eDead;
}

void Thread::SavedRegisters::set(Registers* regs) {
  eip = regs->eip;
  esp = regs->useresp;
  ebp = regs->ebp;
  edi = regs->edi;
  esi = regs->esi;
  ebx = regs->ebx;
}

void Thread::SavedRegisters::copy_to(Registers* regs) {
  regs->eip = eip;
  regs->useresp = esp;
  regs->ebp = ebp;
  regs->edi = edi;
  regs->esi = esi;
  regs->ebx = ebx;
}

