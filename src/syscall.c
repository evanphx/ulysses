// syscall.c -- Defines the implementation of a system call system.
//              Written for JamesM's kernel development tutorials.

#include "syscall.h"
#include "isr.h"

#include "monitor.h"

static void syscall_handler(registers_t *regs);

void sys_monitor_write(const char* p) {
  console.write(p);
}

void sys_monitor_write_hex(int num) {
  console.write_hex(num);
}

void sys_monitor_write_dec(int num) {
  console.write_dec(num);
}

DEFN_SYSCALL1(monitor_write, 0, const char*);
DEFN_SYSCALL1(monitor_write_hex, 1, int);
DEFN_SYSCALL1(monitor_write_dec, 2, int);

static void *syscalls[3] =
{
    (void*)&sys_monitor_write,
    (void*)&sys_monitor_write_hex,
    (void*)&sys_monitor_write_dec,
};
u32int num_syscalls = 3;

void initialise_syscalls()
{
    // Register our syscall handler.
    register_isr_handler(0x80, &syscall_handler);
}

void syscall_handler(registers_t *regs)
{
    console.write("in syscall handler\n");
    // Firstly, check if the requested syscall number is valid.
    // The syscall number is found in EAX.
    if (regs->eax >= num_syscalls)
        return;

    // Get the required syscall location.
    void *location = syscalls[regs->eax];

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
