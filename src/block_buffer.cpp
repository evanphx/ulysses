#include "common.hpp"
#include "block.hpp"

#include "block_buffer.hpp"

#include "kheap.hpp"
#include "cpu.hpp"
#include "scheduler.hpp"

namespace block {
  Buffer* Buffer::for_size(Device* dev, u32 num_bytes) {
    u8* data = (u8*)kmalloc(num_bytes);
    Buffer* buf = new(kheap) Buffer(num_bytes, data, dev);
    return buf;
  }

  void Buffer::fill() {
    if(full_p()) return;
    state_ |= eRequested;
    device_->fulfill(this);
  }

  void Buffer::busy_wait() {
    if(full_p()) return;

    if((state_ & eRequested) == 0) fill();

    if(full_p()) return;

    int st = cpu::enable_interrupts();

    while(!full_p()) cpu::halt();

    cpu::restore_interrupts(st);
  }

  void Buffer::wait() {
    auto token = scheduler.start_io();

    synchronized(lock_) {
      if(full_p()) return;
      if((state_ & eRequested) == 0) fill();

      waiting_task_ = scheduler.current();
    }

    scheduler.io_wait(token);

    // We're back!
    ASSERT(full_p());
  }

  void Buffer::set_full() {
    synchronized(lock_) {
      state_ |= eFull;
      if(waiting_task_) {
        scheduler.make_ready(waiting_task_);
      }
    }
  }
}
