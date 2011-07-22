// paging.hpp -- Defines the interface for and structures relating to paging.
//             Written for JamesM's kernel development tutorials.

#ifndef PAGING_H
#define PAGING_H

#include "common.hpp"
#include "isr.hpp"
#include "fs.hpp"
#include "cpu.hpp"

#define KERNEL_VIRTUAL_BASE 0xC0000000
#define USER_STACK_SIZE 0x800000

namespace x86 {
  struct Page {
    u32 present    : 1;   // Page present in memory
    u32 rw         : 1;   // Read-only if clear, readwrite if set
    u32 user       : 1;   // Supervisor level only if clear
    u32 accessed   : 1;   // Has the page been accessed since last refresh?
    u32 dirty      : 1;   // Has the page been written to since last refresh?
    u32 unused     : 7;   // Amalgamation of unused and reserved bits
    u32 frame      : 20;  // Frame address (shifted right 12 bits)

    void assign(u32 f, bool write, bool kernel) {
      present = 1;
      rw = (write ? 1 : 0);
      user = (kernel ? 0 : 1);
      frame = f;
    }

    void clear() {
      frame = 0;
    }
  };

  struct PageTable {
    Page pages[1024];
  };

  struct PageDirectory {
    /**
      Array of pointers to pagetables.
     **/
    PageTable* tables[1024];
    /**
      Array of pointers to the pagetables above, but gives their *physical*
      location, for loading into the CR3 register.
     **/
    u32 tablesPhysical[1024];

    /**
      The physical address of tablesPhysical. This comes into play
      when we get our kernel heap allocated and the directory
      may be in a different location in virtual memory.
     **/
    u32 physicalAddr;
  };
}

struct Task;

class MemoryMapping {
  u32 address_;
  u32 mem_size_;

  fs::Node* node_;
  u32 offset_;
  u32 file_size_;

  int flags_;

public:
  enum Flags {
    eReadable = 1,
    eWritable = 2,
    eExecutable = 4,
    eAll = 7
  };

  MemoryMapping(u32 address, u32 mem_size, fs::Node* node, u32 offset, u32 size,
                int flags)
    : address_(address)
    , mem_size_(mem_size)
    , node_(node)
    , offset_(offset)
    , file_size_(size)
    , flags_(flags)
  {}

  u32 address() {
    return address_;
  }

  u32 mem_size() {
    return mem_size_;
  }

  u32 end_address() {
    return address_ + mem_size_;
  }

  void enlarge_mem_size(u32 ammount) {
    mem_size_ += ammount;
  }

  fs::Node* node() {
    return node_;
  }

  u32 offset() {
    return offset_;
  }

  u32 file_size() {
    return file_size_;
  }

  bool contains_p(u32 addr) {
    return addr >= address_ && addr < address_ + mem_size_;
  }

  bool readable_p() {
    return (flags_ & eReadable) == eReadable;
  }

  bool writable_p() {
    return (flags_ & eWritable) == eWritable;
  }

  bool fulfill(Task* task, u32 addr);
};

struct VirtualMemory {
  // The kernel's page directory
  x86::PageDirectory* kernel_directory;

  // The current page directory;
  x86::PageDirectory* current_directory;

  u32* frames;
  u32  nframes;

  void init(u32 total_memory, u32 kstart, u32 kend, u32 mem_end);

  void alloc_frame(x86::Page *page, bool is_kernel=false, bool is_writeable=false);

  void alloc_kernel_frame(x86::Page* page, bool writable=false) {
    alloc_frame(page, true, writable);
  }

  void alloc_user_frame(x86::Page* page, bool writable=false) {
    alloc_frame(page, false, writable);
  }

  void free_frame(x86::Page *page);

  void switch_page_directory(x86::PageDirectory *dir);
  x86::Page* get_page(u32 address, int make, x86::PageDirectory* dir);
  x86::Page* get_page(u32* allocp, u32 address, int make, x86::PageDirectory* dir);
  x86::Page* get_kernel_page(u32 address, int make);
  x86::Page* get_current_page(u32 address, int make);
  x86::PageDirectory* clone_directory(x86::PageDirectory* src);
  x86::PageDirectory* clone_current();

  void free_table(x86::PageTable* tbl);
  void free_directory(x86::PageDirectory* dir);

  private:

  x86::PageTable* clone_table(x86::PageTable* src, u32* phys);

// Macros used in the bitset algorithms.
#define INDEX_FROM_BIT(a) (a/(8*4))
#define OFFSET_FROM_BIT(a) (a%(8*4))


  // Static function to set a bit in the frames bitset
  void set_frame(u32 frame_addr) {
    u32 frame = frame_addr/cpu::cPageSize;
    u32 idx = INDEX_FROM_BIT(frame);
    u32 off = OFFSET_FROM_BIT(frame);
    frames[idx] |= (0x1 << off);
  }

  // Static function to clear a bit in the frames bitset
  void clear_frame(u32 frame_addr) {
    u32 frame = frame_addr/cpu::cPageSize;
    u32 idx = INDEX_FROM_BIT(frame);
    u32 off = OFFSET_FROM_BIT(frame);
    frames[idx] &= ~(0x1 << off);
  }

  // Static function to test if a bit is set.
  u32 test_frame(u32 frame_addr) {
    u32 frame = frame_addr/cpu::cPageSize;
    u32 idx = INDEX_FROM_BIT(frame);
    u32 off = OFFSET_FROM_BIT(frame);
    return (frames[idx] & (0x1 << off));
  }

  // Static function to find the first free frame.
  u32 first_frame(bool& ok) {
    ok = false;
    for(u32 i = 0; i < INDEX_FROM_BIT(nframes); i++) {
      // if this set is full, skip.
      if(frames[i] == 0xFFFFFFFF) continue;

      // at least one bit is free here.
      for(u32 j = 0; j < 32; j++) {
        u32 toTest = 0x1 << j;

        if((frames[i] & toTest) == 0) {
          ok = true;
          return i*4*8+j;
        }
      }
    }

    return 0;
  }
};

extern VirtualMemory vmem;

#endif
