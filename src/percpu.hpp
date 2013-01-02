#ifndef PERCPU_HPP
#define PERCPU_HPP

class Thread;

extern "C" u32 read_fs_offset(int offset);
extern "C" u32 set_fs_offset(int offset, u32 val);

struct PerCPU {
  Thread* thread;

  static inline Thread* current_thread() {
    return (Thread*)read_fs_offset(__builtin_offsetof(PerCPU, thread));
  }

  static inline void set_current_thread(Thread* t) {
    set_fs_offset(__builtin_offsetof(PerCPU, thread), (u32)t);
  }
};

#endif
