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

  elf::Loader loader(req);

  if(!loader.load_into(scheduler.process())) {
    regs->eax = -1;
    return 0;
  }

  regs->eax = 0;
  regs->edx = 0; // we don't define a kernel specified fini
  regs->eip = loader.target_ip();
  regs->ds = segments::cUserDS;
  regs->ss = segments::cUserDS;
  regs->cs = segments::cUserCS;
  regs->useresp = loader.new_esp();

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
  scheduler.exit(code);
  return 0;
}

SYSCALL(22, exit_group, int code) {
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

SYSCALL(7, open, const char* name, int mode) {
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

SYSCALL(22, dup, int fd) {
  return scheduler.process()->dup_fd(fd);
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

SYSCALL(18, notimpl, Registers* regs) {
  console.printf("Unimplemented syscall used from %p: %p\n", regs->eip,
                 regs->ebx);
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

struct region {
  int entry;
  u32 base;
  u32 limit;
};

SYSCALL(23, set_thread_area, struct region* reg) {
  if(reg->entry != -1) return 1;
  reg->entry = set_gs(reg->base, reg->limit);
  return 0;
}

SYSCALL(24, rt_sigprocmask, int how, void* set, void* old, int size) {
  return 0;
}

SYSCALL(25, set_tid_address, int* addr) {
  return 0;
}

SYSCALL(26, kill, int pid, int sig) {
  return 0;
}

SYSCALL(27, getpgrp) {
  return scheduler.process_group();
}

SYSCALL(29, geteuid) {
  return scheduler.euid();
}

SYSCALL(30, getppid, int pid) {
  return scheduler.process_group(pid);
}

SYSCALL(31, getcwd, char* buf, int size) {
  buf[0] = '/';
  buf[1] = 0;

  return 0;
}

SYSCALL(32, rt_sigaction, int sig, void* set, void* old, int mask_size) {
  return 0;
}

SYSCALL(33, fcntl, int fd, int cmd, void* arg) {
  console.printf("fcntl of '%d': %d\n", cmd);
  return -1;
}

SYSCALL(34, close, int fd) {
  if(fs::File* file = scheduler.process()->get_file(fd)) {
    return file->close();
  } else {
    return -1;
  }
}


/*
struct stat {
	dev_t st_dev;
	int __st_dev_padding;
	long __st_ino_truncated;
	mode_t st_mode;
	nlink_t st_nlink;
	uid_t st_uid;
	gid_t st_gid;
	dev_t st_rdev;
	int __st_rdev_padding;
	off_t st_size;
	blksize_t st_blksize;
	blkcnt_t st_blocks;
	struct timespec st_atim;
	struct timespec st_mtim;
	struct timespec st_ctim;
	ino_t st_ino;
};
*/

SYSCALL(28, stat, char* path, struct stat* info) {
  console.printf("Trying to stat '%s'\n");
  return -1;
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
      console.printf("bad syscall: %d\n", regs->eax);
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
