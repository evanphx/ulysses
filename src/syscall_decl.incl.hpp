DECL_SYSCALL1(kprint, const char*);
DECL_SYSCALL0(fork);
DECL_SYSCALL0(getpid);
DECL_SYSCALL0(pause);
DECL_SYSCALL1(exit, int);
DECL_SYSCALL1(sleep, int);
DECL_SYSCALL1(wait_any, int*);
DECL_SYSCALL2(open, const char*, int);
DECL_SYSCALL3(read, int, char*, int);
DECL_SYSCALL3(mount, const char*, const char*, const char*);
DECL_SYSCALL3(seek, int, int, int);
DECL_SYSCALL3(write, int, char*, int);
DECL_SYSCALL1(sbrk, int);
DECL_SYSCALL3(getdents, int, void*, int);
DECL_SYSCALL2(channel_connect, int, int);
DECL_SYSCALL0(channel_create);
DECL_SYSCALL3(msg_recv, int, void*, int);
DECL_SYSCALL3(writev, int, struct iovec*, int);
DECL_SYSCALL3(ioctl, int, unsigned long, unsigned long);
DECL_SYSCALL1(brk, u32);
DECL_SYSCALL1(dup, int);
DECL_SYSCALL1(set_thread_area, struct region*);
DECL_SYSCALL4(rt_sigprocmask, int, void*, void*, int);
DECL_SYSCALL1(set_tid_address, int*);
DECL_SYSCALL2(kill, int, int);
DECL_SYSCALL0(getpgrp);
DECL_SYSCALL2(stat, char*, struct stat*);
DECL_SYSCALL0(geteuid);
DECL_SYSCALL1(getppid, int);
DECL_SYSCALL2(getcwd, char*, int);
DECL_SYSCALL4(rt_sigaction, int, void*, void*, int);
DECL_SYSCALL3(fcntl, int, int, void*);
DECL_SYSCALL1(close, int);
