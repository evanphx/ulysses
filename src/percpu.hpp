#ifndef PERCPU_HPP
#define PERCPU_HPP

class Thread;

extern "C" u32 read_fs_offset(int offset);
extern "C" u32 set_fs_offset(int offset, u32 val);

struct PerCPU {
  Thread* thread_;

  static inline Thread* thread() {
    return (Thread*)read_fs_offset(__builtin_offsetof(PerCPU, thread_));
  }

  static inline void set_thread(Thread* t) {
    set_fs_offset(__builtin_offsetof(PerCPU, thread_), (u32)t);
  }
};

#endif
