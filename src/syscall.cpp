// syscall.c -- Defines the implementation of a system call system.
//              Written for JamesM's kernel development tutorials.

#include "syscall.hpp"
#include "isr.hpp"

#include "monitor.hpp"
#include "elf.hpp"

#include "task.hpp"
#include "cpu.hpp"

#include "descriptor_tables.hpp"

void sys_kprint(const char* p) {
  console.printf("%d: %s", scheduler.getpid(), p);
}

void sys_exec(Registers* regs) {
  const char* path = (const char*)regs->ebx;

  fs::Node* test = fs_root->finddir(path, strlen(path));

  if(!test) {
    console.printf("Exec of %s failed, not found.\n", path);
    return;
  }

  u32 new_esp;
  u32 loc = elf::load_node(test, &new_esp);

  regs->eip = loc;
  regs->ds = segments::cUserDS;
  regs->ss = segments::cUserDS;
  regs->cs = segments::cUserCS;
  regs->useresp = new_esp;

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

int sys_open(char* name, int mode) {
  return scheduler.current->open_file(name, mode);
}

int sys_read(int fd, char* buffer, int size) {
  if(fs::File* file = scheduler.current->get_file(fd)) {
    if(size == 0) return 0;
    if(size < 0) return -1;

    return (int)file->read((u8*)buffer, (u32)size);
  } else {
    console.printf("unable to find fd %d\n", fd);
    return -1;
  }
}

int sys_write(int fd, int size, char* buffer) {
  if(fs::File* file = scheduler.current->get_file(fd)) {
    if(size == 0) return 0;
    if(size < 0) return -1;

    return (int)file->write((u8*)buffer, (u32)size);
  } else {
    console.printf("unable to find fd %d\n", fd);
    return -1;
  }
}

int sys_mount(const char* path, const char* fstype, const char* dev) {
  return fs::mount(path, fstype, dev);
}

int sys_seek(int fd, int pos, int whence) {
  if(fs::File* file = scheduler.current->get_file(fd)) {
    file->seek(pos, whence);
    return 0;
  }

  return -1;
}

u32 sys_sbrk(int bytes) {
  u32 addr = scheduler.current->change_heap(bytes);
  console.printf("sbrk = %x\n", addr);
  return addr;
}

const static u32 raw_syscall_base = 1024;

DEFN_SYSCALL1(kprint, 0, const char*);
DEFN_SYSCALL0(fork, 1);
DEFN_SYSCALL0(getpid, 2);
DEFN_SYSCALL0(pause, 3);
DEFN_SYSCALL1(exit, 4, int);
DEFN_SYSCALL1(sleep, 5, int);
DEFN_SYSCALL1(wait_any, 6, int*);
DEFN_SYSCALL2(open, 7, char*, int);
DEFN_SYSCALL3(read, 8, int, char*, int);
DEFN_SYSCALL3(mount, 9, char*, char*, char*);
DEFN_SYSCALL3(seek, 10, int, int, int);
DEFN_SYSCALL3(write, 11, int, int, char*);
DEFN_SYSCALL1(sbrk, 12, int);

DEFN_SYSCALL1(exec, raw_syscall_base + 0, const char*);

static void* syscalls[] = {
    (void*)&sys_kprint,
    (void*)&sys_fork,
    (void*)&sys_getpid,
    (void*)&sys_pause,
    (void*)&sys_exit,
    (void*)&sys_sleep,
    (void*)&sys_wait_any,
    (void*)&sys_open,
    (void*)&sys_read,
    (void*)&sys_mount,
    (void*)&sys_seek,
    (void*)&sys_write,
    (void*)&sys_sbrk
};

static void* raw_syscalls[] = {
    (void*)&sys_exec
};

const static u32 num_syscalls = 13;
const static u32 num_raw_syscalls = 1;

class SyscallDispatcher : public interrupt::Handler {
public:
  void handle(Registers* regs) {

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
        ((void (*)(Registers*))location)(regs);
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
};

void initialise_syscalls() {
  static SyscallDispatcher dispatcher;

  // Register our syscall handler.
  interrupt::register_isr(0x80, &dispatcher);
}

