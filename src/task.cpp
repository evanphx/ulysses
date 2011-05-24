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
#include "fs.hpp"
#include "fs/devfs.hpp"

Scheduler scheduler;

Task::Task(int pid)
  : id(pid)
  , alarm_at(0)
  , exit_code(0)
  , break_mapping(0)
{
  for(int i = 0; i < 16; i++) {
    fds_[i] = 0;
  }
}

void Task::sleep_til(int secs) {
  alarm_at = timer.ticks + timer.secs_to_ticks(secs);
}

bool Task::alarm_expired() {
  return state == Task::eWaiting && 
         alarm_at != 0 &&
         alarm_at <= timer.ticks;
}


void Task::add_mmap(fs::Node* node, u32 offset, u32 size, u32 addr, u32 mem_size,
                    int flags)
{
  MemoryMapping mapping(addr, mem_size, node, offset, size, flags);
  mmaps.append(mapping);
}

int Task::open_file(const char* path, int mode) {
  fs::Node* node = fs::lookup(path+1, strlen(path)-1, fs_root);

  if(!node) return -1;

  // block::Device* dev = block::registry.get(1);
  // fs::Node* node = devfs::main.make_node(dev, path);
  int fd = find_fd();

  fds_[fd] = new(kheap) fs::File(node);

  return fd;
}

fs::File* Task::get_file(int fd) {
  if(fd < 0 || fd >= 16) return 0;
  return fds_[fd];
}

MemoryMapping* Task::find_mapping(u32 addr) {
  Task::MMapList::Iterator i = mmaps.begin();

  while(i.more_p()) {
    MemoryMapping& mmap = i.advance();

    if(mmap.contains_p(addr)) return &mmap;
  }

  return 0;
}

u32 Task::change_heap(int bytes) {
  if(!break_mapping) {
    int flags = MemoryMapping::eAll;
    u32 addr = 0x2000000;
    MemoryMapping mapping(addr, bytes, 0, 0, 0, flags);
    break_mapping = &mmaps.append(mapping);
    return addr;
  }

  u32 ret = break_mapping->end_address();
  break_mapping->enlarge_mem_size(bytes);
  return ret;
}

extern "C" void* save_registers(volatile Task::SavedRegisters*);
extern "C" void restore_registers(volatile Task::SavedRegisters*, u32);
extern "C" void second_return();

void Scheduler::init() {
  // Rather important stuff happening, no interrupts please!
  cpu::disable_interrupts();

  next_pid = 0;
  current = 0;
  ready_queue.init();
  cleanup_queue.init();
  waiting_queue.init();

  // Initialise the first task (kernel task)
  u32 mem = kmalloc_a(KERNEL_STACK_SIZE);

  current = new((void*)mem) Task(next_pid++);
  current->directory = vmem.current_directory;
  current->kernel_stack = mem + KERNEL_STACK_SIZE;

  task0 = current;

  ASSERT(task0->id == 0);

  make_ready(current);

  // Reenable interrupts.
  cpu::enable_interrupts();
}

void Scheduler::cleanup() {
  Task::RunList::Iterator i = cleanup_queue.begin();

  while(i.more_p()) {
    Task* task = i.advance();
    cleanup_queue.unlink(task);

    vmem.free_directory(task->directory);

    kfree(task);
  }
}

void Scheduler::on_tick() {
  Task::RunList::Iterator i = waiting_queue.begin();

  bool schedule = false;
  while(i.more_p()) {
    Task* task = i.advance();

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
  void* val = save_registers((Task::SavedRegisters*)&current->regs);

  // Have we just switched tasks?
  if(val == &second_return) return;

  // Get the next task to run.
  Task* next = current->next_runnable();

  // If we fell off the end of the linked list start again at the beginning.
  if(!next) {
    if(ready_queue.count() == 0) {
      console.printf("NO TASKS TO RUN\n");
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

  current->exit_code = code;
  current->state = Task::eDead;
  cleanup_queue.prepend(current);

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

  current->state = Task::eIOWait;

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
  u32 mem = kmalloc_a(KERNEL_STACK_SIZE);

  Task* new_task = new((void*)mem) Task(next_pid++);
  new_task->directory = directory;
  new_task->kernel_stack = mem + KERNEL_STACK_SIZE;

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

int Scheduler::spawn_thread(void (*func)()) {
  // We are modifying kernel structures, and so cannot be interrupted.
  int st = cpu::disable_interrupts();

  // Clone the address space.
  x86::PageDirectory* directory = vmem.clone_current();

  // Create a new process.
  u32 mem = kmalloc_a(KERNEL_STACK_SIZE);

  Task* new_task = new((void*)mem) Task(next_pid++);
  new_task->directory = directory;
  new_task->kernel_stack = mem + KERNEL_STACK_SIZE;

  ready_queue.append(new_task);

  save_registers(&new_task->regs);
  new_task->regs.eip = (u32)func;
  new_task->regs.esp = new_task->kernel_stack;

  // All finished: Reenable interrupts.
  cpu::restore_interrupts(st);

  // And by convention return the PID of the child.
  return new_task->id;
}

int Scheduler::getpid() {
  return current->id;
}
