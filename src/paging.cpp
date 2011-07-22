// paging.c -- Defines the interface for and structures relating to paging.
//             Written for JamesM's kernel development tutorials.

#include "paging.hpp"
#include "kheap.hpp"
#include "monitor.hpp"
#include "cpu.hpp"
#include "task.hpp"

VirtualMemory vmem = {0, 0, 0, 0};

// Function to allocate a frame.
void VirtualMemory::alloc_frame(x86::Page* page, bool is_kernel, bool is_writeable) {
  if(page->frame != 0) return;
  bool ok = false;

  u32 idx = first_frame(ok);
  if(!ok) kabort();

  set_frame(idx * cpu::cPageSize);

  page->assign(idx, is_writeable, is_kernel);
}

// Function to deallocate a frame.
void VirtualMemory::free_frame(x86::Page* page) {
  u32 frame = page->frame;

  if(!frame) return;
  clear_frame(frame);
  page->clear();
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

  x86::Page* p = vmem.get_current_page(page_address, 1);
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

class PageFault : public interrupt::Handler {
public:
  void handle(Registers* regs) {
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
};

static inline u32 bump(u32* point, u32 size) {
  u32 addr = *point;
  *point += size;
  return addr;
}

void VirtualMemory::init(u32 total_memory, u32 kstart, u32 kend, u32 mem_end) {
  kernel_directory = 0;
  current_directory = 0;

  u32 allocp = mem_end;

  // The size of physical memory. For the moment we 
  // assume it is 16MB big.
  u32 mem_end_page = total_memory;

  nframes = mem_end_page / cpu::cPageSize;
  frames = (u32*)bump(&allocp, INDEX_FROM_BIT(nframes));
  memset((u8int*)frames, 0, INDEX_FROM_BIT(nframes));

  // Let's make a page directory.
  allocp = cpu::page_align(allocp);

  kernel_directory = (x86::PageDirectory*)bump(&allocp, sizeof(x86::PageDirectory));
  memset((u8int*)kernel_directory, 0, sizeof(x86::PageDirectory));
  kernel_directory->physicalAddr = (u32)kernel_directory->tablesPhysical - KERNEL_VIRTUAL_BASE;

  // Ok, first off, boot.s loaded a simple page directory that mapped
  // the first 4M physical address up to KERNEL_VIRTUAL_BASE;
  //
  // So lets go ahead and keep that and add that to the map first.
  //
  // the memory is already in use (it holds the kernel, thats how we got here)
  // and is loaded in from frame 0 up.
  //
  // Therefore alloc_user_frame MUST allocation linearly from 0 upward
  // otherwise our page table won't point to the existing locations.
  u32 page = 0;

  u32 initial_heap_end = mem_end + KHEAP_INITIAL_SIZE;

  for(page = KERNEL_VIRTUAL_BASE;
      page < initial_heap_end;
      page += cpu::cPageSize)
  {
    alloc_user_frame(get_page(&allocp, page, 1, kernel_directory));
  }

  ASSERT(allocp < initial_heap_end);

  static PageFault page_fault;

  // Before we enable paging, we must register our page fault handler.
  interrupt::register_isr(14, &page_fault);

  // Now, enable paging!
  switch_page_directory(kernel_directory);

  // Initialise the kernel heap.
  kheap = Heap::create(allocp,
                       cpu::page_align(initial_heap_end),
                       0xCFFFF000, 0, 0);

  current_directory = clone_directory(kernel_directory);
  switch_page_directory(current_directory);
}

void VirtualMemory::switch_page_directory(x86::PageDirectory* dir) {
  current_directory = dir;

  cpu::set_page_directory(dir->physicalAddr);
  cpu::enable_paging();
}

x86::Page* VirtualMemory::get_page(u32* allocp, u32 address, int make,
                                   x86::PageDirectory* dir)
{
  // Turn the address into an index.
  address /= cpu::cPageSize;
  // Find the page table containing this address.
  u32 table_idx = address / 1024;

  if(dir->tables[table_idx]) { // If this table is already assigned
    return &dir->tables[table_idx]->pages[address%1024];
  } else if(make) {
    *allocp = cpu::page_align(*allocp);

    u32 addr = *allocp;

    u32 tmp = addr - KERNEL_VIRTUAL_BASE;

    *allocp += sizeof(x86::PageTable);

    dir->tables[table_idx] = (x86::PageTable*)addr;

    memset((u8int*)dir->tables[table_idx], 0, cpu::cPageSize);
    dir->tablesPhysical[table_idx] = tmp | 0x7; // PRESENT, RW, US.
    return &dir->tables[table_idx]->pages[address%1024];
  } else {
    return 0;
  }
}


x86::Page* VirtualMemory::get_page(u32 address, int make, x86::PageDirectory* dir) {
  // Turn the address into an index.
  address /= cpu::cPageSize;
  // Find the page table containing this address.
  u32 table_idx = address / 1024;

  if(dir->tables[table_idx]) { // If this table is already assigned
    return &dir->tables[table_idx]->pages[address%1024];
  } else if(make) {
    u32 tmp;
    dir->tables[table_idx] = (x86::PageTable*)kmalloc_ap(sizeof(x86::PageTable), &tmp);
    memset((u8int*)dir->tables[table_idx], 0, cpu::cPageSize);
    dir->tablesPhysical[table_idx] = tmp | 0x7; // PRESENT, RW, US.
    return &dir->tables[table_idx]->pages[address%1024];
  } else {
    return 0;
  }
}

x86::Page* VirtualMemory::get_kernel_page(u32 address, int make) {
  return get_page(address, make, kernel_directory);
}

x86::Page* VirtualMemory::get_current_page(u32 address, int make) {
  return get_page(address, make, current_directory);
}

extern "C" void copy_page_physical(int, int);

x86::PageTable* VirtualMemory::clone_table(x86::PageTable* src, u32 *phys) {
  // Make a new page table, which is page aligned.
  x86::PageTable* table = knew_phys<x86::PageTable>(phys);

  // Ensure that the new table is blank.
  memset((u8int*)table, 0, sizeof(x86::PageTable));

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
    copy_page_physical(  src->pages[i].frame * cpu::cPageSize,
                       table->pages[i].frame * cpu::cPageSize);
  }

  return table;
}

void VirtualMemory::free_table(x86::PageTable* src) {
  for(int i = 0; i < 1024; i++) {
    if(!src->pages[i].frame) continue;
    free_frame(&src->pages[i]);
  }

  kfree(src);
}

x86::PageDirectory* VirtualMemory::clone_directory(x86::PageDirectory* src) {
  u32 phys;
  // Make a new page directory and obtain its physical address.
  x86::PageDirectory* dir = knew_phys<x86::PageDirectory>(&phys);
  // Ensure that it is blank.
  memset((u8int*)dir, 0, sizeof(x86::PageDirectory));

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

void VirtualMemory::free_directory(x86::PageDirectory* dir) {
  for(int i = 0; i < 1024; i++) {
    if(!dir->tables[i]) continue;

    if(kernel_directory->tables[i] != dir->tables[i]) {
      free_table(dir->tables[i]);
    }
  }

  kfree(dir);
}

x86::PageDirectory* VirtualMemory::clone_current() {
  return clone_directory(current_directory);
}
