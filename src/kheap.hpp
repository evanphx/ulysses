// kheap.hpp -- Interface for kernel heap functions, also provides
//            a placement malloc() for use before the heap is 
//            initialised.
//            Written for JamesM's kernel development tutorials.

#ifndef KHEAP_H
#define KHEAP_H

#include "common.hpp"
#include "ordered_array.hpp"

#define KHEAP_START         0xC0000000
#define KHEAP_INITIAL_SIZE  0x100000

#define HEAP_INDEX_SIZE   0x20000
#define HEAP_MAGIC        0x123890AB
#define HEAP_MIN_SIZE     0x70000

/**
   Size information for a hole/block
**/

struct Heap {
  struct header {
    u32 magic;   // Magic number, used for error checking and identification.
    u8 is_hole;   // 1 if this is a hole. 0 if this is a block.
    u32 size;    // size of the block, including the end footer.

    static inline bool less_than(void* a, void* b) {
      return (((header*)a)->size < ((header*)b)->size);
    }
  };

  struct footer {
    u32 magic;     // Magic number, same as in header.
    header* hdr; // Pointer to the block header.
  };

  ordered_array<header*, header> index;
  u32 start_address; // The start of our allocated space.
  u32 end_address;   // The end of our allocated space. May be expanded up to max_address.
  u32 max_address;   // The maximum address the heap can be expanded to.
  u8 supervisor;     // Should extra pages requested by us be mapped as supervisor-only?
  u8 readonly;       // Should extra pages requested by us be mapped as read-only?

  static Heap *create(u32 start, u32 end, u32 max, u8 supervisor, u8 readonly);
  void* alloc(u32 size, u8 page_align);
  void  free(void* p);

  struct Allocation {
    void* virt;
    void* phys;
  };

  Allocation allocate(u32 size, int align=0);
};

extern Heap* kheap;

extern "C" {

/**
   Create a new heap.
**/

/**
   Allocates a contiguous region of memory 'size' in size. If page_align==1, it creates that block starting
   on a page boundary.
**/

/**
   Releases a block allocated with 'alloc'.
**/

/**
   Allocate a chunk of memory, sz in size. If align == 1,
   the chunk must be page-aligned. If phys != 0, the physical
   location of the allocated chunk will be stored into phys.

   This is the internal version of kmalloc. More user-friendly
   parameter representations are available in kmalloc, kmalloc_a,
   kmalloc_ap, kmalloc_p.
**/
u32 kmalloc_int(u32 sz, int align, u32 *phys);

/**
   Allocate a chunk of memory, sz in size. The chunk must be
   page aligned.
**/
u32 kmalloc_a(u32 sz);

void* kmalloc_vp(int sz);

/**
   Allocate a chunk of memory, sz in size. The physical address
   is returned in phys. Phys MUST be a valid pointer to u32!
**/
u32 kmalloc_p(u32 sz, u32 *phys);

/**
   Allocate a chunk of memory, sz in size. The physical address 
   is returned in phys. It must be page-aligned.
**/
u32 kmalloc_ap(u32 sz, u32 *phys);

/**
   General allocation function.
**/
u32 kmalloc(u32 sz);

/**
   General deallocation function.
**/
void kfree(void *p);

}

inline void* operator new(long unsigned int sz) {
  kabort();
  return (void*)-1;
}

inline void* operator new(long unsigned int sz, Heap*) {
  return (void*)kmalloc(sz);
}

// Make placement new work!
inline void* operator new(long unsigned int, void* __p) { return __p; }

template <typename T>
T* knew() {
  return new(kheap) T();
}

template <typename T>
T* knew_array(int size) {
  int max = sizeof(T) * size;
  char* ptr = (char*)kmalloc(max);

  for(char* i = ptr; i < ptr + max; i += sizeof(T)) {
    T* o = (T*)i;
    new(o) T();
  }

  return (T*)ptr;
}


#endif // KHEAP_H
