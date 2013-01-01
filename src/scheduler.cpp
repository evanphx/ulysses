#include "scheduler.hpp"
#include "paging.hpp"
#include "kheap.hpp"
#include "descriptor_tables.hpp"
#include "cpu.hpp"
#include "console.hpp"
#include "timer.hpp"
#include "fs.hpp"
#include "fs/devfs.hpp"

#include "stats.hpp"

#include "keyboard.hpp"

Scheduler scheduler;

extern "C" void* save_registers(volatile Thread::SavedRegisters*);
extern "C" void restore_registers(volatile Thread::SavedRegisters*, u32);

extern "C" void new_thread_tramp();

extern "C" u8 initial_task;

void Scheduler::init() {
  // Rather important stuff happening, no interrupts please!
  cpu::disable_interrupts();

  current = 0;

  for(int i = 0; i < constants::cMaxProcesses; i++) {
    processes_[0] = 0;
  }

  cleanup_.init();

  ready_queue_.init();
  waiting_queue_.init();

  // Initialise the first thread (kernel thread)
  u32 mem = (u32)&initial_task;

  PosixSession init_session(PosixSession::Init);

  // Create process 0, the idle process.
  Process* proc = new(kheap) Process(0,init_session);
  proc->directory = vmem.current_directory;

  processes_[proc->pid()] = proc;
  
  current = proc->new_thread((void*)mem);
  current->directory = vmem.current_directory;
  current->kernel_stack = mem + KERNEL_STACK_SIZE;

  current->state_ = Thread::eReady;

  cpu::set_thread(current);

  ASSERT(current == cpu::read_thread());

  idle_thread_ = current;

  // Reenable interrupts.
  cpu::enable_interrupts();
}

void Scheduler::cleanup() {
  Process::CleanupList::Iterator i = cleanup_.begin();

  while(i.more_p()) {
    Process* proc = i.advance();
    cleanup_.unlink(proc);
    processes_[proc->pid()] = 0;

    vmem.free_directory(proc->directory);

    kfree(proc);
  }
}

void Scheduler::on_tick() {
  bool schedule = false;

  synchronized(lock_) {
    Thread::RunList::Iterator i = waiting_queue_.begin();

    while(i.more_p()) {
      Thread* thread = i.advance();

      if(thread->alarm_expired()) {
        waiting_queue_.unlink(thread);
        make_ready(thread);
        schedule = true;
      }
    }
  }

  if(schedule) switch_thread();
}

void Scheduler::on_idle() {
  synchronized(lock_) {
    cleanup();
  }

  switch_thread();
}

void Scheduler::yield() {
  switch_thread();
}

bool Scheduler::switch_thread() {
  // If we haven't initialised threading yet, just return.
  if(!current) return false;

  // We are modifying kernel structures, and so cannot be interrupted.
  int st = cpu::disable_interrupts();

  if(save_registers(&current->regs)) {
    cpu::restore_interrupts(st);
    return true;
  }

  Thread* next = 0;
  bool switched = false;

  synchronized(lock_) {
    if(ready_queue_.count() == 0) {
      next = idle_thread_;
    } else {
      next = ready_queue_.head();
    }
  }

  ASSERT(next);

  if(next == current) goto done;

  synchronized(lock_) {
    if(next != idle_thread_) {
      // Move it to the end
      ready_queue_.unlink(next);
      ready_queue_.append(next);
    }

    switched = true;

    current = next;

    // Make sure the memory manager knows we've changed page directory.
    vmem.current_directory = current->directory;

    // Change our kernel stack over.
    set_kernel_stack(current->kernel_stack);
  }

  cpu::set_thread(current);

  restore_registers(&current->regs, vmem.current_directory->physicalAddr);

done:
  cpu::restore_interrupts(st);

  return switched;
}

void Scheduler::schedule_hiprio(Thread* thr) {
  if(current == thr) return;

  synchronized(lock_) {
    hiprio_threads_.append(thr);
  }
}

void Scheduler::process_keyboard() {
  if(!console_) return;

  synchronized(lock_) {
    Buffer& buffer = keyboard.buffer();
    if(!buffer.has_data_p()) return;

    console_->inject_bytes(buffer);

    buffer.clear();
  }
}

void Scheduler::exit(int code) {
  ASSERT(getpid() != 0);

  synchronized(lock_) {
    ready_queue_.unlink(current);

    process()->exit(code);
    cleanup_.prepend(current->process());
  }

  switch_thread();
}

int Scheduler::wait_any(int *status) {
  return -1;

}

void Scheduler::sleep(int secs) {
  ASSERT(getpid() != 0);

  synchronized(lock_) {
    current->sleep_til(secs);
    ready_queue_.unlink(current);
    make_wait(current);
  }

  switch_thread();
}

Scheduler::IOToken Scheduler::start_io() {
  synchronized(lock_) {
    current->state_ = Thread::eIOPending;
  }

  return IOToken();
}

void Scheduler::io_wait(IOToken) {
  ASSERT(getpid() != 0);

  synchronized(lock_) {
    // Between the time of start_io and io_wait, the IO
    // was completed! We don't even need to wait.
    if(current->state_ != Thread::eIOPending) {
      stats.fast_io.inc();
      return;
    }

    stats.slow_io.inc();

    current->state_ = Thread::eIOWait;
    ready_queue_.unlink(current);
  }

  switch_thread();
}

int Scheduler::fork() {
  Process* proc = 0;

  synchronized(lock_) {
    // Clone the address space.
    x86::PageDirectory* directory = vmem.clone_current();

    // Create a new process.
    proc = new(kheap) Process(new_pid(), session());
    processes_[proc->pid()] = proc;

    proc->directory = directory;
  }

  u32 mem = kmalloc_a(KERNEL_STACK_SIZE);

  Thread* new_thread = proc->new_thread((void*)mem);
  new_thread->directory = proc->directory;

  new_thread->kernel_stack = mem + KERNEL_STACK_SIZE;

  make_ready(new_thread);

  if(save_registers(&new_thread->regs) == 0) {
    // And by convention return the PID of the child.
    return proc->pid();
  } else {
    // We are the child - by convention return 0.
    return 0;
  }
}

extern "C" void start_new_thread(void (*func)(), Thread* th) {
  scheduler.start_new_thread(func, th);
}

void Scheduler::start_new_thread(void (*func)(), Thread* th) {
  cpu::enable_interrupts();

  func();

  synchronized(lock_) {
    ready_queue_.unlink(current);
  }

  // TODO cleanup th somehow
  switch_thread();
}

int Scheduler::spawn_init(void (*func)(void)) {
  // We are modifying kernel structures, and so cannot be interrupted.
  int st = cpu::disable_interrupts();

  Process* proc = 0;

  synchronized(lock_) {
    x86::PageDirectory* directory = vmem.new_directory();

    // Create a new process.
    proc = new(kheap) Process(1, session());
    processes_[1] = proc;

    proc->directory = directory;
  }

  u32 mem = kmalloc_a(KERNEL_STACK_SIZE);

  Thread* new_thread = proc->new_thread((void*)mem);
  new_thread->directory = proc->directory;
  new_thread->kernel_stack = mem + KERNEL_STACK_SIZE;

  proc->add_thread(new_thread);

  make_ready(new_thread);

  save_registers(&new_thread->regs);
  new_thread->regs.eip = (u32)new_thread_tramp;
  new_thread->regs.esp = new_thread->kernel_stack;
  new_thread->regs.ebp = (u32)func;
  new_thread->regs.ebx = (u32)new_thread;

  // All finished: Reenable interrupts.
  cpu::restore_interrupts(st);

  return proc->pid();
}

Thread* Scheduler::spawn_thread(void (*func)()) {
  // We are modifying kernel structures, and so cannot be interrupted.
  int st = cpu::disable_interrupts();

  // Clone the address space.
  x86::PageDirectory* directory;
  
  synchronized(lock_) {
    directory = vmem.clone_current();
  }

  // Create a new process.
  u32 mem = kmalloc_a(KERNEL_STACK_SIZE);

  Thread* new_thread = 0;

  synchronized(lock_) {
    new_thread = process()->new_thread((void*)mem);
  }

  new_thread->directory = directory;
  new_thread->kernel_stack = mem + KERNEL_STACK_SIZE;

  save_registers(&new_thread->regs);
  new_thread->regs.eip = (u32)new_thread_tramp;
  new_thread->regs.esp = new_thread->kernel_stack;
  new_thread->regs.ebp = (u32)func;
  new_thread->regs.ebx = (u32)new_thread;

  // All finished: Reenable interrupts.
  cpu::restore_interrupts(st);

  return new_thread;
}

int Scheduler::new_pid() {
  for(int i = 0; i < constants::cMaxProcesses; i++) {
    if(!processes_[i]) return i;
  }

  kputs("no more room for procesess!");
  kabort();

  return 0; // compiler snack
}


int Scheduler::getpid() {
  return process()->pid();
}

Process* Scheduler::find_process(int pid) {
  return processes_[pid];
}

int Scheduler::process_group(int pid) {
  if(!pid) return session().pgrp();

  Process* proc = find_process(pid);
  if(proc) return proc->session().pgrp();

  return -1;
}
