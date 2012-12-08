// syscall.c -- Defines the implementation of a system call system.
//              Written for JamesM's kernel development tutorials.

#include "syscall.hpp"
#include "isr.hpp"

#include "monitor.hpp"
#include "elf.hpp"

#include "thread.hpp"
#include "scheduler.hpp"
#include "cpu.hpp"

#include "descriptor_tables.hpp"

#include "ipc.hpp"
#include "process.hpp"

SYSCALL(0, kprint, const char* p) {
  console.printf("%d: %s", scheduler.getpid(), p);
  return 0;
}

SYSCALL(17, exec, Registers* regs) {
  const char* path = (const char*)regs->ebx;
  const char** argp = (const char**)regs->ecx;
  const char** envp = (const char**)regs->edx;

  elf::Request req(path, argp, envp);

  if(!req.load_file()) {
    regs->eax = -1;
    console.printf("Exec of %s failed, not found.\n", path);
    return 0;
  }

  if(!elf::load_node(req)) {
    regs->eax = -1;
    return 0;
  }

  regs->eax = 0;
  regs->eip = req.target_ip;
  regs->ds = segments::cUserDS;
  regs->ss = segments::cUserDS;
  regs->cs = segments::cUserCS;
  regs->useresp = req.new_esp;

  console.printf("ready to execute '%s' at %p...\n", path, regs->eip);

  return 0;
}

DEFN_SYSCALL3(exec, 17, const char*, const char**, const char**);

SYSCALL(1, fork) {
  return scheduler.fork();
}

SYSCALL(2, getpid) {
  return scheduler.getpid();
}

SYSCALL(3, pause) {
  scheduler.switch_task();
  return 0;
}

SYSCALL(4, exit, int code) {
  console.printf("exit called\n");
  scheduler.exit(code);
  return 0;
}

SYSCALL(5, sleep, int seconds) {
  scheduler.sleep(seconds);
  return seconds;
}

SYSCALL(6, wait_any, int* status) {
  return scheduler.wait_any(status);
}

SYSCALL(7, open, char* name, int mode) {
  return scheduler.process()->open_file(name, mode);
}

SYSCALL(8, read, int fd, char* buffer, int size) {
  if(fs::File* file = scheduler.process()->get_file(fd)) {
    if(size == 0) return 0;
    if(size < 0) return -1;

    return (int)file->read((u8*)buffer, (u32)size);
  } else {
    console.printf("unable to find fd %d\n", fd);
    return -1;
  }
}

SYSCALL(11, write, int fd, char* buffer, int size) {
  if(fs::File* file = scheduler.process()->get_file(fd)) {
    if(size == 0) return 0;
    if(size < 0) return -1;

    return (int)file->write((u8*)buffer, (u32)size);
  } else {
    console.printf("unable to find fd %d\n", fd);
    return -1;
  }
}

SYSCALL(9, mount, const char* path, const char* fstype, const char* dev) {
  return fs::mount(path, fstype, dev);
}

SYSCALL(10, seek, int fd, int pos, int whence) {
  if(fs::File* file = scheduler.process()->get_file(fd)) {
    file->seek(pos, whence);
    return 0;
  }

  return -1;
}

SYSCALL(12, sbrk, int bytes) {
  u32 addr = scheduler.process()->change_heap(bytes);
  return addr;
}

SYSCALL(21, brk, u32 target) {
  return (int)scheduler.process()->set_brk(target);
}

SYSCALL(13, getdents, int fd, void* dp, int count) {
  if(fs::File* file = scheduler.process()->get_file(fd)) {
    return file->get_entries(dp, count);
  }

  return -1;
}

SYSCALL(14, channel_connect, int pid, int cid) {
  return -1; // ipc::channel_connect(pid, cid);
}

SYSCALL(15, channel_create) {
  return -1; // ipc::channel_create();
}

SYSCALL(16, msg_recv, int cid, void* msg, int len) {
  return -1; // ipc::msg_recv(cid, msg, len);
}

SYSCALL(18, notimpl) {
  console.printf("Unimplemented syscall used\n");
  return -1;
}

struct iovec { void *base; u32 len; };

SYSCALL(19, writev, int fd, struct iovec* vec, int veccnt) {
  if(fs::File* file = scheduler.process()->get_file(fd)) {

    int out = 0;
    for(int i = 0; i < veccnt; i++) {
      out += file->write((u8*)vec[i].base, vec[i].len);
    }

    return out;
  } else {
    console.printf("unable to find fd %d\n", fd);
    return -1;
  }
  return -1;
}

SYSCALL(20, ioctl, int fd, unsigned long req, unsigned long arg) {
  if(fs::File* file = scheduler.process()->get_file(fd)) {
    return file->ioctl(req, arg);
  } else {
    return -1;
  }
}

const char* syscall_name(int idx);

#include "syscall_tramp.incl.hpp"

class SyscallDispatcher : public interrupt::Handler {
public:
  void handle(Registers* regs) {

    /*
    console.printf("in syscall handler: %s (%d) (total %d)\n",
                   syscall_names[regs->eax], regs->eax, num_syscalls);
    */

    // Firstly, check if the requested syscall number is valid.
    // The syscall number is found in EAX.


    if(regs->eax < num_syscalls) {
      void* location = syscalls[regs->eax];

      ((void (*)(Registers*))location)(regs);
    } else {
      console.printf("bad syscall\n");
    }
  }
};

void initialise_syscalls() {
  static SyscallDispatcher dispatcher;

  // Register our syscall handler.
  interrupt::register_isr(0x80, &dispatcher);
}

#include "syscall_impl.incl.hpp"

const char* syscall_name(int idx) {
  return syscall_names[idx];
}
