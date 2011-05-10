// paging.c -- Defines the interface for and structures relating to paging.
//             Written for JamesM's kernel development tutorials.

#include "paging.hpp"
#include "kheap.hpp"
#include "monitor.hpp"
#include "cpu.hpp"
#include "task.hpp"

VirtualMemory vmem = {0, 0, 0, 0};

extern "C" {
  // Defined in kheap.c
  extern u32 placement_address;
}

// Function to allocate a frame.
void VirtualMemory::alloc_frame(page *page, bool is_kernel, bool is_writeable) {
  if(page->frame != 0) return;
  u32 idx = first_frame();
  if(idx == (u32)-1) {
    // PANIC! no free frames!!
    kabort();
  }

  set_frame(idx*0x1000);
  page->present = 1;
  page->rw = (is_writeable ? 1 : 0);
  page->user = (is_kernel ? 0 : 1);
  page->frame = idx;
}

// Function to deallocate a frame.
void VirtualMemory::free_frame(page *page) {
  u32 frame = page->frame;

  if(!frame) return;
  clear_frame(frame);
  page->frame = 0x0;
}

bool MemoryMapping::fulfill(Task* task, u32 request) {
  u32 page_address = request & cpu::cPageMask;
  u32 request_offset = page_address - address_;

  u32 target_offset = offset_ + request_offset;

  u32 target_size;
  if(request_offset > file_size_) {
    target_size = 0;
  } else {
    target_size = file_size_ - request_offset;
    // We only fill one page, so clamp the target_size
    if(target_size > cpu::cPageSize) {
      target_size = cpu::cPageSize;
    }
  }

  u32 zero_fill_size = cpu::cPageSize - target_size;

  page* p = vmem.get_current_page(page_address, 1);
  vmem.alloc_user_frame(p, writable_p());

  /*
  console.printf("on-demand mapped %x to %x for %x (offset=%d, size=%d)\n",
                 page_address, p->frame, request,
                 target_offset, target_size);
  */

  if(target_size > 0) {
    node_->read(target_offset, target_size, (u8*)page_address);
  }

  if(zero_fill_size > 0) {
    memset((u8*)page_address + target_size, 0, zero_fill_size);
  }

  return true;
}

static void page_fault(registers_t *regs) {
  // A page fault has occurred.
  // The faulting address is stored in the CR2 register.
  u32 faulting_address = cpu::fault_address();

  MemoryMapping* mmap = scheduler.current->find_mapping(faulting_address);

  // The error code gives us details of what happened.
  bool present = !(regs->err_code & 0x1); // Page not present
  bool rw = regs->err_code & 0x2;           // Write operation?
  bool us = regs->err_code & 0x4;           // Processor was in user-mode?
  bool reserved = regs->err_code & 0x8;     // Overwritten CPU-reserved bits of page entry?

  // Ok, this is for a mapping in the current process.
  if(mmap) {
    if(!rw || mmap->writable_p()) {
      if(mmap->fulfill(scheduler.current, faulting_address)) return;
    }
  }

  // int id = regs->err_code & 0x10;          // Caused by an instruction fetch?

  // Output an error message.
  console.write("Page fault! ( ");
  if(present) console.write("present ");
  if(rw) console.write("read-only ");
  if(us) console.write("user-mode ");
  if(reserved) console.write("reserved ");
  console.printf(") at 0x%x\n", faulting_address);

  console.printf("ds: %x\n", regs->ds);
  console.printf("edi: %x, esi: %x, ebp: %x, esp: %x\n",
                 regs->edi, regs->esi, regs->ebp, regs->esp);
  console.printf("ebx: %x, edx: %x, ecx: %x, eax: %x\n",
                 regs->ebx, regs->edx, regs->ecx, regs->eax);
  console.printf("int: %d, err: %d\n", regs->int_no, regs->err_code);
  console.printf("eip: %x, cs: %x, eflags: %x, useresp: %x, ss: %x\n",
                 regs->eip, regs->cs, regs->eflags,
                 regs->useresp, regs->ss);

  PANIC("Page fault");
}

void VirtualMemory::init(u32 total_memory) {
  kernel_directory = 0;
  current_directory = 0;

  // The size of physical memory. For the moment we 
  // assume it is 16MB big.
  u32 mem_end_page = total_memory;

  nframes = mem_end_page / 0x1000;
  frames = (u32*)kmalloc(INDEX_FROM_BIT(nframes));
  memset((u8int*)frames, 0, INDEX_FROM_BIT(nframes));

  // Let's make a page directory.
  kernel_directory = (page_directory*)kmalloc_a(sizeof(page_directory));
  memset((u8int*)kernel_directory, 0, sizeof(page_directory));
  kernel_directory->physicalAddr = (u32)kernel_directory->tablesPhysical;

  // Map some pages in the kernel heap area.
  // Here we call get_page but not alloc_frame. This causes page_table's 
  // to be created where necessary. We can't allocate frames yet because they
  // they need to be identity mapped first below, and yet we can't increase
  // placement_address between identity mapping and enabling the heap!
  for(u32 i = KHEAP_START; i < KHEAP_START+KHEAP_INITIAL_SIZE; i += 0x1000) {
    get_page(i, 1, kernel_directory);
  }

  // We need to identity map (phys addr = virt addr) from
  // 0x0 to the end of used memory, so we can access this
  // transparently, as if paging wasn't enabled.
  // NOTE that we use a while loop here deliberately.
  // inside the loop body we actually change placement_address
  // by calling kmalloc(). A while loop causes this to be
  // computed on-the-fly rather than once at the start.
  // Allocate a lil' bit extra so the kernel heap can be
  // initialised properly.

  for(u32 i = 0; i < placement_address + 0x1000; i += 0x1000) {
    // Kernel code is readable but not writeable from userspace.
    alloc_user_frame(get_page(i, 1, kernel_directory));
  }

  // Now allocate those pages we mapped earlier.
  for(u32 i = KHEAP_START; i < KHEAP_START+KHEAP_INITIAL_SIZE; i += 0x1000) {
    alloc_user_frame(get_page(i, 1, kernel_directory));
  }

  // Before we enable paging, we must register our page fault handler.
  register_isr_handler(14, page_fault);

  // Now, enable paging!
  switch_page_directory(kernel_directory);

  // Initialise the kernel heap.
  kheap = Heap::create(KHEAP_START, KHEAP_START+KHEAP_INITIAL_SIZE, 0xCFFFF000, 0, 0);

  current_directory = clone_directory(kernel_directory);
  switch_page_directory(current_directory);
}

void VirtualMemory::switch_page_directory(page_directory *dir) {
  current_directory = dir;

  cpu::set_page_directory(dir->physicalAddr);
  cpu::enable_paging();
}

page* VirtualMemory::get_page(u32 address, int make, page_directory *dir) {
  // Turn the address into an index.
  address /= 0x1000;
  // Find the page table containing this address.
  u32 table_idx = address / 1024;

  if(dir->tables[table_idx]) { // If this table is already assigned
    return &dir->tables[table_idx]->pages[address%1024];
  } else if(make) {
    u32 tmp;
    dir->tables[table_idx] = (page_table*)kmalloc_ap(sizeof(page_table), &tmp);
    memset((u8int*)dir->tables[table_idx], 0, 0x1000);
    dir->tablesPhysical[table_idx] = tmp | 0x7; // PRESENT, RW, US.
    return &dir->tables[table_idx]->pages[address%1024];
  } else {
    return 0;
  }
}

page* VirtualMemory::get_kernel_page(u32 address, int make) {
  return get_page(address, make, kernel_directory);
}

page* VirtualMemory::get_current_page(u32 address, int make) {
  return get_page(address, make, current_directory);
}

extern "C" void copy_page_physical(int, int);

page_table* VirtualMemory::clone_table(page_table *src, u32 *physAddr) {
  // Make a new page table, which is page aligned.
  page_table *table = (page_table*)kmalloc_ap(sizeof(page_table), physAddr);

  // Ensure that the new table is blank.
  memset((u8int*)table, 0, sizeof(page_table));

  // For every entry in the table...
  for(int i = 0; i < 1024; i++) {
    // If the source entry has a frame associated with it...
    if(!src->pages[i].frame) continue;

    // Get a new frame.
    alloc_user_frame(&table->pages[i]);
    // Clone the flags from source to destination.
    if(src->pages[i].present)  table->pages[i].present = 1;
    if(src->pages[i].rw)       table->pages[i].rw = 1;
    if(src->pages[i].user)     table->pages[i].user = 1;
    if(src->pages[i].accessed) table->pages[i].accessed = 1;
    if(src->pages[i].dirty)    table->pages[i].dirty = 1;

    // Physically copy the data across. This function is in process.s.
    copy_page_physical(src->pages[i].frame*0x1000, table->pages[i].frame*0x1000);
  }

  return table;
}

void VirtualMemory::free_table(page_table* src) {
  for(int i = 0; i < 1024; i++) {
    if(!src->pages[i].frame) continue;
    free_frame(&src->pages[i]);
  }

  kfree(src);
}

page_directory* VirtualMemory::clone_directory(page_directory *src) {
  u32 phys;
  // Make a new page directory and obtain its physical address.
  page_directory *dir = (page_directory*)kmalloc_ap(sizeof(page_directory), &phys);
  // Ensure that it is blank.
  memset((u8int*)dir, 0, sizeof(page_directory));

  // Get the offset of tablesPhysical from the start of the page_directory structure.
  u32 offset = (u32)dir->tablesPhysical - (u32)dir;

  // Then the physical address of dir->tablesPhysical is:
  dir->physicalAddr = phys + offset;

  // Go through each page table. If the page table is in the kernel directory, do not make a new copy.
  for(int i = 0; i < 1024; i++) {
    if(!src->tables[i]) continue;

    if(kernel_directory->tables[i] == src->tables[i]) {
      // It's in the kernel, so just use the same pointer.
      dir->tables[i] = src->tables[i];
      dir->tablesPhysical[i] = src->tablesPhysical[i];
    } else {
      // Copy the table.
      u32 phys;
      dir->tables[i] = clone_table(src->tables[i], &phys);
      dir->tablesPhysical[i] = phys | 0x07;
    }
  }
  return dir;
}

void VirtualMemory::free_directory(page_directory* dir) {
  for(int i = 0; i < 1024; i++) {
    if(!dir->tables[i]) continue;

    if(kernel_directory->tables[i] != dir->tables[i]) {
      free_table(dir->tables[i]);
    }
  }

  kfree(dir);
}

page_directory* VirtualMemory::clone_current() {
  return clone_directory(current_directory);
}
