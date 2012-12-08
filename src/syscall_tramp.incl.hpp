void _syscall_tramp_kprint(Registers* regs) {
  TRACE_START_SYSCALL(0);
  regs->eax = SYSCALL_NAME(kprint)((const char*)regs->ebx);
  TRACE_END_SYSCALL(0);
}
void _syscall_tramp_fork(Registers* regs) {
  TRACE_START_SYSCALL(1);
  regs->eax = SYSCALL_NAME(fork)();
  TRACE_END_SYSCALL(1);
}
void _syscall_tramp_getpid(Registers* regs) {
  TRACE_START_SYSCALL(2);
  regs->eax = SYSCALL_NAME(getpid)();
  TRACE_END_SYSCALL(2);
}
void _syscall_tramp_pause(Registers* regs) {
  TRACE_START_SYSCALL(3);
  regs->eax = SYSCALL_NAME(pause)();
  TRACE_END_SYSCALL(3);
}
void _syscall_tramp_exit(Registers* regs) {
  TRACE_START_SYSCALL(4);
  regs->eax = SYSCALL_NAME(exit)((int)regs->ebx);
  TRACE_END_SYSCALL(4);
}
void _syscall_tramp_sleep(Registers* regs) {
  TRACE_START_SYSCALL(5);
  regs->eax = SYSCALL_NAME(sleep)((int)regs->ebx);
  TRACE_END_SYSCALL(5);
}
void _syscall_tramp_wait_any(Registers* regs) {
  TRACE_START_SYSCALL(6);
  regs->eax = SYSCALL_NAME(wait_any)((int*)regs->ebx);
  TRACE_END_SYSCALL(6);
}
void _syscall_tramp_open(Registers* regs) {
  TRACE_START_SYSCALL(7);
  regs->eax = SYSCALL_NAME(open)((char*)regs->ebx, (int)regs->ecx);
  TRACE_END_SYSCALL(7);
}
void _syscall_tramp_read(Registers* regs) {
  TRACE_START_SYSCALL(8);
  regs->eax = SYSCALL_NAME(read)((int)regs->ebx, (char*)regs->ecx, (int)regs->edx);
  TRACE_END_SYSCALL(8);
}
void _syscall_tramp_mount(Registers* regs) {
  TRACE_START_SYSCALL(9);
  regs->eax = SYSCALL_NAME(mount)((const char*)regs->ebx, (const char*)regs->ecx, (const char*)regs->edx);
  TRACE_END_SYSCALL(9);
}
void _syscall_tramp_seek(Registers* regs) {
  TRACE_START_SYSCALL(10);
  regs->eax = SYSCALL_NAME(seek)((int)regs->ebx, (int)regs->ecx, (int)regs->edx);
  TRACE_END_SYSCALL(10);
}
void _syscall_tramp_write(Registers* regs) {
  TRACE_START_SYSCALL(11);
  regs->eax = SYSCALL_NAME(write)((int)regs->ebx, (char*)regs->ecx, (int)regs->edx);
  TRACE_END_SYSCALL(11);
}
void _syscall_tramp_sbrk(Registers* regs) {
  TRACE_START_SYSCALL(12);
  regs->eax = SYSCALL_NAME(sbrk)((int)regs->ebx);
  TRACE_END_SYSCALL(12);
}
void _syscall_tramp_getdents(Registers* regs) {
  TRACE_START_SYSCALL(13);
  regs->eax = SYSCALL_NAME(getdents)((int)regs->ebx, (void*)regs->ecx, (int)regs->edx);
  TRACE_END_SYSCALL(13);
}
void _syscall_tramp_channel_connect(Registers* regs) {
  TRACE_START_SYSCALL(14);
  regs->eax = SYSCALL_NAME(channel_connect)((int)regs->ebx, (int)regs->ecx);
  TRACE_END_SYSCALL(14);
}
void _syscall_tramp_channel_create(Registers* regs) {
  TRACE_START_SYSCALL(15);
  regs->eax = SYSCALL_NAME(channel_create)();
  TRACE_END_SYSCALL(15);
}
void _syscall_tramp_msg_recv(Registers* regs) {
  TRACE_START_SYSCALL(16);
  regs->eax = SYSCALL_NAME(msg_recv)((int)regs->ebx, (void*)regs->ecx, (int)regs->edx);
  TRACE_END_SYSCALL(16);
}
void _syscall_tramp_exec(Registers* regs) {
  TRACE_START_SYSCALL(17);
  SYSCALL_NAME(exec)(regs);
  TRACE_END_SYSCALL(17);
}
void _syscall_tramp_notimpl(Registers* regs) {
  TRACE_START_SYSCALL(18);
  regs->eax = SYSCALL_NAME(notimpl)();
  TRACE_END_SYSCALL(18);
}
void _syscall_tramp_writev(Registers* regs) {
  TRACE_START_SYSCALL(19);
  regs->eax = SYSCALL_NAME(writev)((int)regs->ebx, (struct iovec*)regs->ecx, (int)regs->edx);
  TRACE_END_SYSCALL(19);
}
void _syscall_tramp_ioctl(Registers* regs) {
  TRACE_START_SYSCALL(20);
  regs->eax = SYSCALL_NAME(ioctl)((int)regs->ebx, (unsigned long)regs->ecx, (unsigned long)regs->edx);
  TRACE_END_SYSCALL(20);
}
void _syscall_tramp_brk(Registers* regs) {
  TRACE_START_SYSCALL(21);
  regs->eax = SYSCALL_NAME(brk)((u32)regs->ebx);
  TRACE_END_SYSCALL(21);
}
static void* syscalls[] = {
  (void*)&_syscall_tramp_kprint,
  (void*)&_syscall_tramp_fork,
  (void*)&_syscall_tramp_getpid,
  (void*)&_syscall_tramp_pause,
  (void*)&_syscall_tramp_exit,
  (void*)&_syscall_tramp_sleep,
  (void*)&_syscall_tramp_wait_any,
  (void*)&_syscall_tramp_open,
  (void*)&_syscall_tramp_read,
  (void*)&_syscall_tramp_mount,
  (void*)&_syscall_tramp_seek,
  (void*)&_syscall_tramp_write,
  (void*)&_syscall_tramp_sbrk,
  (void*)&_syscall_tramp_getdents,
  (void*)&_syscall_tramp_channel_connect,
  (void*)&_syscall_tramp_channel_create,
  (void*)&_syscall_tramp_msg_recv,
  (void*)&_syscall_tramp_exec,
  (void*)&_syscall_tramp_notimpl,
  (void*)&_syscall_tramp_writev,
  (void*)&_syscall_tramp_ioctl,
  (void*)&_syscall_tramp_brk,
  0
};
const static u32 num_syscalls = 22;
static const char* syscall_names[] = {
  "kprint",
  "fork",
  "getpid",
  "pause",
  "exit",
  "sleep",
  "wait_any",
  "open",
  "read",
  "mount",
  "seek",
  "write",
  "sbrk",
  "getdents",
  "channel_connect",
  "channel_create",
  "msg_recv",
  "exec",
  "notimpl",
  "writev",
  "ioctl",
  "brk",
  0
};
