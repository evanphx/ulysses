// kheap.c -- Kernel heap functions, also provides
//            a placement malloc() for use before the heap is 
//            initialised.
//            Written for JamesM's kernel development tutorials.

#include "kheap.h"
#include "paging.h"

Heap* kheap = 0;

/*
Allocation Heap::allocate(u32 size, int align=0) {
  if(align && sz < 0x1000) {
    sz = 0x1000;
  }

  void *addr = kheap->alloc(sz, (u8)align);

  page *page = vmem.get_kernel_page((u32)addr, 0);
  void* phys = page->frame*0x1000 + ((u32)addr&0xFFF);

  Allocation alloc = {addr, phys};

  return alloc;
}
*/

extern "C" {

// end is defined in the linker script.
extern u32 end;
u32 placement_address = (u32)&end;

u32 kmalloc_int(u32 sz, int align, u32 *phys) {
  if(kheap != 0) {
    if(align && sz < 0x1000) {
      sz = 0x1000;
    }

    void *addr = kheap->alloc(sz, (u8)align);
    if(phys != 0) {
      page *page = vmem.get_kernel_page((u32)addr, 0);
      *phys = page->frame*0x1000 + ((u32)addr&0xFFF);
    }

    return (u32)addr;
  } else {
    if(align == 1 && (placement_address & 0xFFFFF000)) {
      // Align the placement address;
      placement_address &= 0xFFFFF000;
      placement_address += 0x1000;
    }

    if (phys) {
      *phys = placement_address;
    }
    u32 tmp = placement_address;
    placement_address += sz;
    return tmp;
  }
}

void kfree(void *p) {
  kheap->free(p);
}

u32 kmalloc_a(u32 sz) {
  return kmalloc_int(sz, 1, 0);
}

void* kmalloc_vp(int sz) {
  void* p = (void*)kmalloc_int(sz, 0, 0);
  return p;
}

void* krealloc_vp(void* ptr, int sz) {
  void* new_ptr = kmalloc_vp(sz);
  memcpy((u8*)new_ptr, (const u8*)ptr, sz);
  return new_ptr;
}

u32 kmalloc_p(u32 sz, u32 *phys) {
  return kmalloc_int(sz, 0, phys);
}

u32 kmalloc_ap(u32 sz, u32 *phys) {
  return kmalloc_int(sz, 1, phys);
}

u32 kmalloc(u32 sz) {
  return kmalloc_int(sz, 0, 0);
}

static void expand(u32 new_size, Heap *heap) {
  // Sanity check.
  ASSERT(new_size > heap->end_address - heap->start_address);

  // Get the nearest following page boundary.
  if(new_size&0xFFFFF000 != 0) {
    new_size &= 0xFFFFF000;
    new_size += 0x1000;
  }

  // Make sure we are not overreaching ourselves.
  ASSERT(heap->start_address+new_size <= heap->max_address);

  // This should always be on a page boundary.
  u32 old_size = heap->end_address-heap->start_address;

  u32 i = old_size;
  while(i < new_size) {
    vmem.alloc_frame( vmem.get_kernel_page(heap->start_address+i, 1),
        (heap->supervisor)?1:0, (heap->readonly)?0:1);
    i += 0x1000 /* page size */;
  }

  heap->end_address = heap->start_address+new_size;
}

static u32 contract(u32 new_size, Heap *heap) {
  // Sanity check.
  ASSERT(new_size < heap->end_address-heap->start_address);

  // Get the nearest following page boundary.
  if(new_size&0x1000) {
    new_size &= 0x1000;
    new_size += 0x1000;
  }

  // Don't contract too far!
  if(new_size < HEAP_MIN_SIZE) new_size = HEAP_MIN_SIZE;

  u32 old_size = heap->end_address-heap->start_address;
  u32 i = old_size - 0x1000;
  while(new_size < i) {
    vmem.free_frame(vmem.get_kernel_page(heap->start_address+i, 0));
    i -= 0x1000;
  }

  heap->end_address = heap->start_address + new_size;
  return new_size;
}

static s32 find_smallest_hole(u32 size, u8 page_align, Heap *heap) {
  // Find the smallest hole that will fit.
  u32 iterator = 0;
  while(iterator < heap->index.size) {
    Heap::header *header = heap->index.lookup(iterator);
    // If the user has requested the memory be page-aligned
    if(page_align > 0) {
      // Page-align the starting point of this header.
      u32 location = (u32)header;
      s32 offset = 0;
      if((location+sizeof(Heap::header)) & 0xFFFFF000 != 0)
        offset = 0x1000 /* page size */  - (location+sizeof(Heap::header))%0x1000;
      s32 hole_size = (s32)header->size - offset;
      // Can we fit now?
      if(hole_size >= (s32)size) break;
    } else if (header->size >= size) {
      break;
    }
    iterator++;
  }
  // Why did the loop exit?
  if(iterator == heap->index.size) {
    return -1; // We got to the end and didn't find anything.
  } else {
    return iterator;
  }
}

Heap* Heap::create(u32 start, u32 end_addr, u32 max, u8 supervisor, u8 readonly) {
  Heap *heap = (Heap*)kmalloc(sizeof(Heap));

  // All our assumptions are made on startAddress and endAddress being page-aligned.
  ASSERT(start%0x1000 == 0);
  ASSERT(end_addr%0x1000 == 0);

  // Initialise the index.
  heap->index = place_ordered_array<Heap::header*, Heap::header>((void*)start, HEAP_INDEX_SIZE);

  // Shift the start address forward to resemble where we can start putting data.
  start += sizeof(void*)*HEAP_INDEX_SIZE;

  // Make sure the start address is page-aligned.
  if(start & 0xFFFFF000 != 0) {
    start &= 0xFFFFF000;
    start += 0x1000;
  }

  // Write the start, end and max addresses into the heap structure.
  heap->start_address = start;
  heap->end_address = end_addr;
  heap->max_address = max;
  heap->supervisor = supervisor;
  heap->readonly = readonly;

  // We start off with one large hole in the index.
  Heap::header *hole = (Heap::header *)start;
  hole->size = end_addr-start;
  hole->magic = HEAP_MAGIC;
  hole->is_hole = 1;
  heap->index.insert(hole);

  return heap;
}

void* Heap::alloc(u32 size, u8 page_align) {
  // Make sure we take the size of header/footer into account.
  u32 new_size = size + sizeof(Heap::header) + sizeof(Heap::footer);
  // Find the smallest hole that will fit.
  s32 iterator = find_smallest_hole(new_size, page_align, this);

  if(iterator == -1) { // If we didn't find a suitable hole
    // Save some previous data.
    u32 old_length = end_address - start_address;
    u32 old_end_address = end_address;

    // We need to allocate some more space.
    expand(old_length+new_size, this);
    u32 new_length = end_address-start_address;

    // Find the endmost header. (Not endmost in size, but in location).
    iterator = 0;
    // Vars to hold the index of, and value of, the endmost header found so far.
    u32 idx = -1;
    u32 value = 0x0;

    while(iterator < index.size) {
      u32 tmp = (u32)index.lookup(iterator);
      if(tmp > value) {
        value = tmp;
        idx = iterator;
      }
      iterator++;
    }

    // If we didn't find ANY headers, we need to add one.
    if(idx == -1) {
      Heap::header *header = (Heap::header *)old_end_address;
      header->magic = HEAP_MAGIC;
      header->size = new_length - old_length;
      header->is_hole = 1;
      Heap::footer *footer = (Heap::footer *) (old_end_address + header->size - sizeof(Heap::footer));
      footer->magic = HEAP_MAGIC;
      footer->hdr = header;
      index.insert(header);
    } else {
      // The last header needs adjusting.
      Heap::header *header = index.lookup(idx);
      header->size += new_length - old_length;
      // Rewrite the footer.
      Heap::footer *footer = (Heap::footer *) ( (u32)header + header->size - sizeof(Heap::footer) );
      footer->hdr = header;
      footer->magic = HEAP_MAGIC;
    }
    // We now have enough space. Recurse, and call the function again.
    return alloc(size, page_align);
  }

  Heap::header *orig_hole_header = index.lookup(iterator);
  u32 orig_hole_pos = (u32)orig_hole_header;
  u32 orig_hole_size = orig_hole_header->size;

  // Here we work out if we should split the hole we found into two parts.
  // Is the original hole size - requested hole size less than the overhead for adding a new hole?
  if(orig_hole_size - new_size < sizeof(Heap::header) + sizeof(Heap::footer)) {
    // Then just increase the requested size to the size of the hole we found.
    size += orig_hole_size-new_size;
    new_size = orig_hole_size;
  }

  // If we need to page-align the data, do it now and make a new hole in front of our block.
  if(page_align && orig_hole_pos&0xFFFFF000) {
    u32 new_location   = orig_hole_pos + 0x1000 /* page size */ - (orig_hole_pos&0xFFF) - sizeof(Heap::header);
    Heap::header *hole_header = (Heap::header *)orig_hole_pos;
    hole_header->size     = 0x1000 /* page size */ - (orig_hole_pos&0xFFF) - sizeof(Heap::header);
    hole_header->magic    = HEAP_MAGIC;
    hole_header->is_hole  = 1;
    Heap::footer *hole_footer = (Heap::footer *) ( (u32)new_location - sizeof(Heap::footer) );
    hole_footer->magic    = HEAP_MAGIC;
    hole_footer->hdr   = hole_header;
    orig_hole_pos         = new_location;
    orig_hole_size        = orig_hole_size - hole_header->size;

  } else {
    // Else we don't need this hole any more, delete it from the index.
    index.remove(iterator);
  }

  // Overwrite the original header...
  Heap::header *block_header  = (Heap::header *)orig_hole_pos;
  block_header->magic     = HEAP_MAGIC;
  block_header->is_hole   = 0;
  block_header->size      = new_size;
  // ...And the footer
  Heap::footer *block_footer  = (Heap::footer *) (orig_hole_pos + sizeof(Heap::header) + size);
  block_footer->magic     = HEAP_MAGIC;
  block_footer->hdr    = block_header;

  // We may need to write a new hole after the allocated block.
  // We do this only if the new hole would have positive size...
  if(orig_hole_size - new_size > 0) {
    Heap::header *hole_header = (Heap::header *) (orig_hole_pos + sizeof(Heap::header) + size + sizeof(Heap::footer));
    hole_header->magic    = HEAP_MAGIC;
    hole_header->is_hole  = 1;
    hole_header->size     = orig_hole_size - new_size;
    Heap::footer *hole_footer = (Heap::footer *) ( (u32)hole_header + orig_hole_size - new_size - sizeof(Heap::footer) );
    if((u32)hole_footer < end_address) {
      hole_footer->magic = HEAP_MAGIC;
      hole_footer->hdr = hole_header;
    }
    // Put the new hole in the index;
    index.insert(hole_header);
  }

  // ...And we're done!
  return (void *) ((u32)block_header+sizeof(Heap::header));
}

void Heap::free(void *p) {
  // Exit gracefully for null pointers.
  if (p == 0) return;

  // Get the header and footer associated with this pointer.
  Heap::header *header = (Heap::header*) ( (u32)p - sizeof(Heap::header) );
  Heap::footer *footer = (Heap::footer*) ( (u32)header + header->size - sizeof(Heap::footer) );

  // Sanity checks.
  ASSERT(header->magic == HEAP_MAGIC);
  ASSERT(footer->magic == HEAP_MAGIC);

  // Make us a hole.
  header->is_hole = 1;

  // Do we want to add this header into the 'free holes' index?
  char do_add = 1;

  // Unify left
  // If the thing immediately to the left of us is a footer...
  Heap::footer *test_footer = (Heap::footer*) ( (u32)header - sizeof(Heap::footer) );
  if(test_footer->magic == HEAP_MAGIC && test_footer->hdr->is_hole == 1) {
    u32 cache_size = header->size; // Cache our current size.
    header = test_footer->hdr;     // Rewrite our header with the new one.
    footer->hdr = header;          // Rewrite our footer to point to the new header.
    header->size += cache_size;       // Change the size.
    do_add = 0;                       // Since this header is already in the index, we don't want to add it again.
  }

  // Unify right
  // If the thing immediately to the right of us is a header...
  Heap::header *test_header = (Heap::header*) ( (u32)footer + sizeof(Heap::footer) );
  if(test_header->magic == HEAP_MAGIC && test_header->is_hole) {
    header->size += test_header->size; // Increase our size.
    test_footer = (Heap::footer*) ( (u32)test_header + // Rewrite it's footer to point to our header.
        test_header->size - sizeof(Heap::footer) );
    footer = test_footer;
    // Find and remove this header from the index.
    u32 iterator = 0;
    while((iterator < index.size) &&
        (index.lookup(iterator) != test_header)) {
      iterator++;
    }

    // Make sure we actually found the item.
    ASSERT(iterator < index.size);
    // Remove it.
    index.remove(iterator);
  }

  // If the footer location is the end address, we can contract.
  if((u32)footer+sizeof(Heap::footer) == end_address) {
    u32 old_length = end_address-start_address;
    u32 new_length = contract( (u32)header - start_address, this);

    // Check how big we will be after resizing.
    if(header->size - (old_length-new_length) > 0) {
      // We will still exist, so resize us.
      header->size -= old_length-new_length;
      footer = (Heap::footer*) ( (u32)header + header->size - sizeof(Heap::footer) );
      footer->magic = HEAP_MAGIC;
      footer->hdr = header;
    } else {
      // We will no longer exist :(. Remove us from the index.
      u32 iterator = 0;
      while((iterator < index.size) &&
          (index.lookup(iterator) != test_header)) {
        iterator++;
      }
      // If we didn't find ourselves, we have nothing to remove.
      if(iterator < index.size) {
        index.remove(iterator);
      }
    }
  }

  // If required, add us to the index.
  if(do_add == 1) {
    index.insert(header);
  }

}

}
