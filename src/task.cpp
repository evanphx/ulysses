// 
// task.c - Implements the functionality needed to multitask.
//          Written for JamesM's kernel development tutorials.
//

#include "task.hpp"
#include "paging.hpp"
#include "kheap.hpp"
#include "descriptor_tables.hpp"
#include "cpu.hpp"
#include "console.hpp"
#include "timer.hpp"

Scheduler scheduler;

void Task::sleep_til(int secs) {
  alarm_at = timer.ticks + timer.secs_to_ticks(secs);
}

bool Task::alarm_expired() {
  return state == Task::eWaiting && 
         alarm_at != 0 &&
         alarm_at <= timer.ticks;
}

// Some externs are needed to access members in paging.c...
extern u32 initial_esp;
extern "C" u32 read_eip();

extern "C" void* save_registers(volatile Task::SavedRegisters*);
extern "C" void restore_registers(volatile Task::SavedRegisters*, u32);
extern "C" void second_return();

static void move_stack(void *new_stack_start, u32 size) {
  // Allocate some space for the new stack.
  for(u32 i = (u32)new_stack_start;
      i >= ((u32)new_stack_start-size);
      i -= 0x1000)
  {
    // General-purpose stack is in user-mode.
    vmem.alloc_frame(
        vmem.get_current_page(i, 1),
        0 /* User mode */,
        1 /* Is writable */ );
  }

  cpu::flush_tbl();

  // Old ESP and EBP, read from registers.
  u32 old_stack_pointer; asm volatile("mov %%esp, %0" : "=r" (old_stack_pointer));
  u32 old_base_pointer;  asm volatile("mov %%ebp, %0" : "=r" (old_base_pointer));

  // Offset to add to old stack addresses to get a new stack address.
  u32 offset            = (u32)new_stack_start - initial_esp;

  // New ESP
  u32 new_stack_pointer = old_stack_pointer + offset;

  int copy_size = initial_esp - old_stack_pointer;

  // Copy the stack.
  memcpy((u8*)new_stack_pointer, (u8*)old_stack_pointer, copy_size);
}

void Scheduler::init() {
  // Rather important stuff happening, no interrupts please!
  cpu::disable_interrupts();

  next_pid = 0;
  current = 0;
  ready_queue.init();
  cleanup_queue.init();
  waiting_queue.init();

  // Relocate the stack so we know where it is.
  move_stack((void*)0xE0000000, 0x2000);

  // Initialise the first task (kernel task)
  current = new(kheap) Task(next_pid++);
  current->directory = vmem.current_directory;
  current->kernel_stack = kmalloc_a(KERNEL_STACK_SIZE);

  make_ready(current);

  // Reenable interrupts.
  cpu::enable_interrupts();
}

void Scheduler::cleanup() {
  Task* task = cleanup_queue.head();

  while(task) {
    Task* nxt = task->next;
    cleanup_queue.unlink(task);

    vmem.free_directory(task->directory);

    kfree((void*)task->kernel_stack);
    kfree(task);

    task = nxt;
  }
}

void Scheduler::on_tick() {
  Task* task = waiting_queue.head();
  bool schedule = false;

  while(task) {
    Task* nxt = task->next;

    if(task->alarm_expired()) {
      make_ready(task);
      schedule = true;
    }

    task = nxt;
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
  void* val = save_registers((Task::SavedRegisters*)&current->regs);

  // Have we just switched tasks?
  if(val == &second_return) return;

  // Get the next task to run.
  Task* next = current->next;

  // If we fell off the end of the linked list start again at the beginning.
  if(!next) next = ready_queue.head();

  if(next == current) return;

  current = next;

  // Make sure the memory manager knows we've changed page directory.
  vmem.current_directory = current->directory;

  // Change our kernel stack over.
  set_kernel_stack(current->kernel_stack+KERNEL_STACK_SIZE);

  // Copy the registers in current->regs back to the machine
  // and jump to the eip held in regs.eip.
  //
  // This will cause save_registers to return for a second time and
  // the return value will be &second_return;
  restore_registers(&current->regs, vmem.current_directory->physicalAddr);
}

void Scheduler::exit() {
  // We are modifying kernel structures, and so cannot be interrupted.
  int st = cpu::disable_interrupts();

  if(current->list) {
    current->list->unlink(current);
  }

  current->state = Task::eDead;
  cleanup_queue.prepend(current);

  cpu::restore_interrupts(st);

  switch_task();
}

void Scheduler::sleep(int secs) {
  // We are modifying kernel structures, and so cannot be interrupted.
  int st = cpu::disable_interrupts();

  current->sleep_til(secs);

  make_wait(current);

  cpu::restore_interrupts(st);

  switch_task();
}

Task::Task(int pid)
  : id(pid)
  , list(0)
  , prev(0)
  , next(0)
{}

int Scheduler::fork() {
  // We are modifying kernel structures, and so cannot be interrupted.
  int st = cpu::disable_interrupts();

  // Clone the address space.
  page_directory* directory = vmem.clone_current();

  // Create a new process.
  Task* new_task = new(kheap) Task(next_pid++);
  new_task->directory = directory;
  new_task->kernel_stack = kmalloc_a(KERNEL_STACK_SIZE);

  ready_queue.append(new_task);

  if(save_registers(&new_task->regs) != &second_return) {
    // All finished: Reenable interrupts.
    cpu::restore_interrupts(st);

    // And by convention return the PID of the child.
    return new_task->id;
  } else {
    // We are the child - by convention return 0.
    return 0;
  }
}

int Scheduler::getpid() {
  return current->id;
}

void Scheduler::switch_to_user_mode() {
  // Set up our kernel stack.
  set_kernel_stack(current->kernel_stack+KERNEL_STACK_SIZE);

  // Set up a stack structure for switching to user mode.
  asm volatile("  \
      cli; \
      mov $0x23, %ax; \
      mov %ax, %ds; \
      mov %ax, %es; \
      mov %ax, %fs; \
      mov %ax, %gs; \
      \
      \
      mov %esp, %eax; \
      pushl $0x23; \
      pushl %esp; \
      pushf; \
      pop %eax; \
      mov $0x200, %ecx; \
      or %ecx, %eax; \
      push %eax; \
      pushl $0x1B; \
      push $1f; \
      iret; \
      1: \
      "); 

}
