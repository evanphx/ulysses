#include "scheduler.hpp"
#include "paging.hpp"
#include "kheap.hpp"
#include "descriptor_tables.hpp"
#include "cpu.hpp"
#include "console.hpp"
#include "timer.hpp"
#include "fs.hpp"
#include "fs/devfs.hpp"

Scheduler scheduler;

extern "C" void* save_registers(volatile Thread::SavedRegisters*);
extern "C" void restore_registers(volatile Thread::SavedRegisters*, u32);
extern "C" void second_return();

extern "C" u8 initial_task;

void Scheduler::init() {
  // Rather important stuff happening, no interrupts please!
  cpu::disable_interrupts();

  next_pid = 0;
  current = 0;

  processes_.init();
  cleanup_.init();

  ready_queue.init();
  waiting_queue.init();

  // Initialise the first thread (kernel thread)
  u32 mem = (u32)&initial_task;

  Process* proc = new(kheap) Process(next_pid++);
  proc->directory = vmem.current_directory;

  processes_.append(proc);
  
  current = new((void*)mem) Thread(proc);
  current->directory = vmem.current_directory;
  current->kernel_stack = mem + KERNEL_STACK_SIZE;

  task0 = current;

  make_ready(current);

  // Reenable interrupts.
  cpu::enable_interrupts();
}

void Scheduler::cleanup() {
  Process::CleanupList::Iterator i = cleanup_.begin();

  while(i.more_p()) {
    Process* proc = i.advance();
    cleanup_.unlink(proc);
    processes_.unlink(proc);

    vmem.free_directory(proc->directory);

    kfree(proc);
  }
}

void Scheduler::on_tick() {
  Thread::RunList::Iterator i = waiting_queue.begin();

  bool schedule = false;
  while(i.more_p()) {
    Thread* task = i.advance();

    if(task->alarm_expired()) {
      waiting_queue.unlink(task);
      make_ready(task);
      schedule = true;
    }
  }

  if(schedule) switch_task();
}

void Scheduler::switch_task() {
  // If we haven't initialised tasking yet, just return.
  if(!current) return;

  // Save the current registers into the current.
  //
  // This routine will return twice. Once after we initially call it
  // and other time when the current task is being resumed. We detect
  // that happening by checking if the returned value is &second_return;
  void* val = save_registers((Thread::SavedRegisters*)&current->regs);

  // Have we just switched tasks?
  if(val == &second_return) return;

  // Get the next task to run.
  Thread* next = current->next_runnable();

  // If we fell off the end of the linked list start again at the beginning.
  if(!next) {
    if(ready_queue.count() == 0) {
      console.printf("NO Threads TO RUN\n");
      kabort();
    }

    next = ready_queue.head();
  }

  ASSERT(next);

  if(next == current) return;

  current = next;

  // Make sure the memory manager knows we've changed page directory.
  vmem.current_directory = current->directory;

  // Change our kernel stack over.
  set_kernel_stack(current->kernel_stack);

  // Copy the registers in current->regs back to the machine
  // and jump to the eip held in regs.eip.
  //
  // This will cause save_registers to return for a second time and
  // the return value will be &second_return;
  restore_registers(&current->regs, vmem.current_directory->physicalAddr);
}

void Scheduler::exit(int code) {
  // We are modifying kernel structures, and so cannot be interrupted.
  int st = cpu::disable_interrupts();

  ASSERT(current != task0);

  ready_queue.unlink(current);

  process()->exit(code);
  cleanup_.prepend(current->process());

  cpu::restore_interrupts(st);

  switch_task();
}

int Scheduler::wait_any(int *status) {
  // We are modifying kernel structures, and so cannot be interrupted.
  int st = cpu::disable_interrupts();

  cpu::restore_interrupts(st);
  return -1;

}

void Scheduler::sleep(int secs) {
  // We are modifying kernel structures, and so cannot be interrupted.
  int st = cpu::disable_interrupts();

  current->sleep_til(secs);

  ASSERT(current != task0);

  ready_queue.unlink(current);
  make_wait(current);

  cpu::restore_interrupts(st);

  switch_task();
}

void Scheduler::io_wait() {
  // We are modifying kernel structures, and so cannot be interrupted.
  cpu::disable_interrupts();

  current->state = Thread::eIOWait;

  ASSERT(current != task0);

  ready_queue.unlink(current);

  cpu::enable_interrupts();

  ASSERT(cpu::interrupts_enabled_p());

  switch_task();

  cpu::disable_interrupts();
}

int Scheduler::fork() {
  // We are modifying kernel structures, and so cannot be interrupted.
  int st = cpu::disable_interrupts();

  // Clone the address space.
  x86::PageDirectory* directory = vmem.clone_current();

  // Create a new process.
  Process* proc = new(kheap) Process(next_pid++);
  processes_.append(proc);

  proc->directory = directory;

  u32 mem = kmalloc_a(KERNEL_STACK_SIZE);

  Thread* new_task = new((void*)mem) Thread(proc);
  new_task->directory = directory;

  new_task->kernel_stack = mem + KERNEL_STACK_SIZE;

  ready_queue.append(new_task);

  if(save_registers(&new_task->regs) != &second_return) {
    // All finished: Reenable interrupts.
    cpu::restore_interrupts(st);

    // And by convention return the PID of the child.
    return proc->pid();
  } else {
    // We are the child - by convention return 0.
    return 0;
  }
}

int Scheduler::spawn_thread(void (*func)()) {
  // We are modifying kernel structures, and so cannot be interrupted.
  int st = cpu::disable_interrupts();

  // Clone the address space.
  x86::PageDirectory* directory = vmem.clone_current();

  // Create a new process.
  u32 mem = kmalloc_a(KERNEL_STACK_SIZE);

  Thread* new_task = new((void*)mem) Thread(process());
  new_task->directory = directory;
  new_task->kernel_stack = mem + KERNEL_STACK_SIZE;

  ready_queue.append(new_task);

  save_registers(&new_task->regs);
  new_task->regs.eip = (u32)func;
  new_task->regs.esp = new_task->kernel_stack;

  // All finished: Reenable interrupts.
  cpu::restore_interrupts(st);

  // And by convention return the PID of the child.
  return process()->pid();
}

int Scheduler::getpid() {
  return process()->pid();
}
