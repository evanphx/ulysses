#ifndef PROCESS_HPP
#define PROCESS_HPP

#include "thread.hpp"
#include "paging.hpp"
#include "list.hpp"
#include "fs.hpp"
#include "list.hpp"

class Process {
public:
  typedef sys::ExternalList<MemoryMapping> MMapList;

  enum Lists {
    cAll = 0,
    cCleanup = 1,
    cTotal = 2
  };

  const static u32 cDefaultMMapStart =  0x1000000;
  const static u32 cDefaultBreakStart = 0x2000000;
  const static u32 cDefaultBreakSize  = 1024 * 1024;

  typedef sys::List<Process, cAll> AllList;
  typedef sys::List<Process, cCleanup> CleanupList;

  sys::ListNode<Process> lists[cTotal];

private:
  sys::ExternalList<Thread*> threads_;
  int pid_;

  MMapList mmaps_;

  MemoryMapping* break_mapping_;

  fs::File* fds_[16];
  ipc::Channel* channels_[16];

  bool alive_;
  int exit_code_;

  int thread_ids_;

  u32 next_mmap_start_;

public:
  x86::PageDirectory* directory;

  int pid() {
    return pid_;
  }

  int find_fd() {
    for(int i = 0; i < 16; i++) {
      if(fds_[i] == 0) return i;
    }

    return -1;
  }

  int add_channel(ipc::Channel* chan) {
    for(int i = 0; i < 16; i++) {
      if(channels_[i] == 0) {
        channels_[i] = chan;
        return i;
      }
    }

    return -1;
  }

  void add_thread(Thread* thr) {
    threads_.append(thr);
  }

  void exit(int code);

  Process(int pid);

  void add_mmap(fs::Node* node, u32 offset, u32 size, u32 addr,
                u32 mem_size, int flags);
  MemoryMapping* find_mapping(u32 addr);
  u32 change_heap(int bytes);
  u32 set_brk(u32 target);

  void position_brk(u32 fin);

  int open_file(const char* name, int mode);
  int dup_fd(int fd);

  fs::File* get_file(int fd);

  Thread* new_thread(void* placed);

  u32 new_mmap_region(u32 size);

  void print_mmaps();
};

#endif
