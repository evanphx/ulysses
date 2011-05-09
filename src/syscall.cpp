// syscall.c -- Defines the implementation of a system call system.
//              Written for JamesM's kernel development tutorials.

#include "syscall.hpp"
#include "isr.hpp"

#include "monitor.hpp"
#include "elf.hpp"

#include "task.hpp"
#include "cpu.hpp"

static void syscall_handler(registers_t *regs);

void sys_kprint(const char* p) {
  console.printf("%d: %s", scheduler.getpid(), p);
}

void sys_exec(registers_t* regs) {
  const char* path = (const char*)regs->ebx;

  fs::Node* test = fs_root->finddir(path);

  u32 loc = elf::load_node(test);

  regs->eip = loc;
  return;
}

int sys_fork() {
  return scheduler.fork();
}

int sys_getpid() {
  return scheduler.getpid();
}

void sys_pause() {
  scheduler.switch_task();
}

void sys_exit(int code) {
  scheduler.exit(code);
}

void sys_sleep(int seconds) {
  scheduler.sleep(seconds);
}

void sys_wait_any(int* status) {
  scheduler.wait_any(status);
}

const static u32 raw_syscall_base = 1024;

DEFN_SYSCALL1(kprint, 0, const char*);
DEFN_SYSCALL0(fork, 1);
DEFN_SYSCALL0(getpid, 2);
DEFN_SYSCALL0(pause, 3);
DEFN_SYSCALL1(exit, 4, int);
DEFN_SYSCALL1(sleep, 5, int);
DEFN_SYSCALL1(wait_any, 6, int*);

DEFN_SYSCALL1(exec, raw_syscall_base + 0, const char*);

static void* syscalls[] = {
    (void*)&sys_kprint,
    (void*)&sys_fork,
    (void*)&sys_getpid,
    (void*)&sys_pause,
    (void*)&sys_exit,
    (void*)&sys_sleep,
    (void*)&sys_wait_any
};

static void* raw_syscalls[] = {
    (void*)&sys_exec
};

const static u32 num_syscalls = 7;
const static u32 num_raw_syscalls = 1;

void initialise_syscalls() {
  // Register our syscall handler.
  register_isr_handler(0x80, &syscall_handler);
}

void syscall_handler(registers_t* regs) {
  // console.printf("in syscall handler: %d (total %d)\n", regs->eax, num_syscalls);
  void* location;

  // Firstly, check if the requested syscall number is valid.
  // The syscall number is found in EAX.
  if(regs->eax >= num_syscalls) {
    if(regs->eax >= raw_syscall_base) {
      u32 rebase = regs->eax - raw_syscall_base;
      if(rebase >= num_raw_syscalls) {
        console.printf("Unknown syscall: %d\n", regs->eax);
        regs->eax = 1;
        return;
      }

      location = raw_syscalls[rebase];
      ((isr_t)location)(regs);
    } else {
      console.printf("Unknown syscall: %d\n", regs->eax);
      regs->eax = 1;
      return;
    }
  } else {
    // Get the required syscall location.
    location = syscalls[regs->eax];

    // We don't know how many parameters the function wants, so we just
    // push them all onto the stack in the correct order. The function will
    // use all the parameters it wants, and we can pop them all back off afterwards.
    int ret;
    asm volatile (" \
        push %1; \
        push %2; \
        push %3; \
        push %4; \
        push %5; \
        call *%6; \
        pop %%ebx; \
        pop %%ebx; \
        pop %%ebx; \
        pop %%ebx; \
        pop %%ebx; \
        " : "=a" (ret) : "r" (regs->edi), "r" (regs->esi), "r" (regs->edx), "r" (regs->ecx), "r" (regs->ebx), "r" (location));
    regs->eax = ret;
  }
}
