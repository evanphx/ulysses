// paging.h -- Defines the interface for and structures relating to paging.
//             Written for JamesM's kernel development tutorials.

#ifndef PAGING_H
#define PAGING_H

#include "common.h"
#include "isr.h"

struct page {
  u32int present    : 1;   // Page present in memory
  u32int rw         : 1;   // Read-only if clear, readwrite if set
  u32int user       : 1;   // Supervisor level only if clear
  u32int accessed   : 1;   // Has the page been accessed since last refresh?
  u32int dirty      : 1;   // Has the page been written to since last refresh?
  u32int unused     : 7;   // Amalgamation of unused and reserved bits
  u32int frame      : 20;  // Frame address (shifted right 12 bits)
};

struct page_table {
  page pages[1024];
};

struct page_directory {
  /**
    Array of pointers to pagetables.
   **/
  page_table* tables[1024];
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

struct VirtualMemory {
  // The kernel's page directory
  page_directory* kernel_directory;

  // The current page directory;
  page_directory* current_directory;

  u32* frames;
  u32  nframes;

  void init(u32 total_memory);
  void alloc_frame(page *page, int is_kernel, int is_writeable);
  void free_frame(page *page);

  void switch_page_directory(page_directory *dir);
  page* get_page(u32 address, int make, page_directory *dir);
  page* get_kernel_page(u32 address, int make);
  page* get_current_page(u32 address, int make);
  page_directory* clone_directory(page_directory *src);
  page_directory* clone_current();

  void free_table(page_table* tbl);
  void free_directory(page_directory* dir);

  private:

  page_table* clone_table(page_table* src, u32* phys);

// Macros used in the bitset algorithms.
#define INDEX_FROM_BIT(a) (a/(8*4))
#define OFFSET_FROM_BIT(a) (a%(8*4))


  // Static function to set a bit in the frames bitset
  void set_frame(u32 frame_addr) {
    u32 frame = frame_addr/0x1000;
    u32 idx = INDEX_FROM_BIT(frame);
    u32 off = OFFSET_FROM_BIT(frame);
    frames[idx] |= (0x1 << off);
  }

  // Static function to clear a bit in the frames bitset
  void clear_frame(u32 frame_addr) {
    u32 frame = frame_addr/0x1000;
    u32 idx = INDEX_FROM_BIT(frame);
    u32 off = OFFSET_FROM_BIT(frame);
    frames[idx] &= ~(0x1 << off);
  }

  // Static function to test if a bit is set.
  u32 test_frame(u32 frame_addr) {
    u32 frame = frame_addr/0x1000;
    u32 idx = INDEX_FROM_BIT(frame);
    u32 off = OFFSET_FROM_BIT(frame);
    return (frames[idx] & (0x1 << off));
  }

  // Static function to find the first free frame.
  u32 first_frame() {
    for(u32 i = 0; i < INDEX_FROM_BIT(nframes); i++) {
      if(frames[i] != 0xFFFFFFFF) { // nothing free, exit early.
        // at least one bit is free here.
        for(u32 j = 0; j < 32; j++) {
          u32 toTest = 0x1 << j;
          if(!(frames[i]&toTest)) {
            return i*4*8+j;
          }
        }
      }
    }

    return (u32)-1;
  }
};

extern VirtualMemory vmem;

#endif
