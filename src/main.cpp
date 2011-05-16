// main.c -- Defines the C-code kernel entry point, calls initialisation routines.
//           Made for JamesM's tutorials <www.jamesmolloy.co.uk>

#include "monitor.hpp"
#include "descriptor_tables.hpp"
#include "timer.hpp"
#include "paging.hpp"
#include "multiboot.hpp"
#include "fs.hpp"
#include "initrd.hpp"
#include "task.hpp"
#include "syscall.hpp"
#include "keyboard.hpp"
#include "pci.hpp"
#include "rtc.hpp"
#include "elf.hpp"
#include "block.hpp"
#include "fs/devfs.hpp"
#include "fs/ext2.hpp"

#include "cpu.hpp"

extern "C" {

extern u32 placement_address;
u32 initrd_location;

void kmain2();

static void show_cpuid() {
  u32 ebx, edx, ecx;
  u32 code = 0;

  asm volatile("cpuid" : "=b"(ebx),"=d"(edx),"=c"(ecx) : "a"(code));

  console.write("CPUID: ");
  console.put((ebx & 0x000000ff) >> 0);
  console.put((ebx & 0x0000ff00) >> 8);
  console.put((ebx & 0x00ff0000) >> 16);
  console.put((ebx & 0xff000000) >> 24);
  console.put((edx & 0x000000ff) >> 0);
  console.put((edx & 0x0000ff00) >> 8);
  console.put((edx & 0x00ff0000) >> 16);
  console.put((edx & 0xff000000) >> 24);
  console.put((ecx & 0x000000ff) >> 0);
  console.put((ecx & 0x0000ff00) >> 8);
  console.put((ecx & 0x00ff0000) >> 16);
  console.put((ecx & 0xff000000) >> 24);
  console.write("\n");
}

/* static inline void rdtsc(dword *upper, dword *lower) { */
  /* asm volatile("rdtsc\n" : "=a"(*lower), "=d"(*upper)); */
/* } */

int kmain(struct multiboot *mboot_ptr, u32 initial_stack) {
  // Initialise all the ISRs and segmentation
  init_descriptor_tables();
  // Initialise the screen (by clearing it)
  // console.clear();
  console.setup();

  u32 mem_total;
  if(mboot_ptr->flags & 1) {
    mem_total = (mboot_ptr->mem_lower * 1024) +
                (mboot_ptr->mem_upper * 1024);

    if(mem_total > 1048576) {
      console.printf("mem total: %dMB\n", mem_total / 1048576);
    } else {
      console.printf("mem total: %dB\n", mem_total);
    }

  } else {
    console.printf("memory unknown, default to 16M\n");
    mem_total = 0x1000000;
  }

  // Initialise the PIT to 100Hz
  cpu::interrupts_on = 1;
  cpu::enable_interrupts();

  timer.init(SLICE_HZ);

  show_cpuid();

  // Find the location of our initial ramdisk.
  ASSERT(mboot_ptr->mods_count > 0);
  initrd_location = *((u32*)mboot_ptr->mods_addr);
  u32 initrd_end = *(u32*)(mboot_ptr->mods_addr+4);
  // Don't trample our module with placement accesses, please!
  placement_address = initrd_end;

  console.printf("initrd: 0x%x - 0x%x\n", initrd_location, initrd_end-1);

  // Start paging.
  vmem.init(mem_total);

  // Start multitasking.
  scheduler.init();

  // Setup a new stack and jump to it.

  // Allocate some space for the new stack.
  u32 sp = 0xE0000000;
  u32 size = cpu::cPageSize * 2;

  for(u32 i = sp; i >= sp-size; i -= cpu::cPageSize) {
    vmem.alloc_kernel_frame(vmem.get_current_page(i, 1), true);
  }

  cpu::flush_tbl();

  asm volatile(
    "mov %0, %%esp\n"
    "mov %0, %%ebp\n"
    "jmp kmain2\n"
  : : "r" (sp));

  return 0;
}

void kmain2() {
  // Initialise the initial ramdisk, and set it as the filesystem root.
  fs_root = initrd::fs.init(initrd_location);

  keyboard.init();

  initialise_syscalls();

  fs::registry.init();
  devfs::main.init();
  ext2::init();

  block::registry.init();

  pci_bus.init();

  // block::registry.print();

  if(syscall_fork() == 0) {
    /* console.write("Switching to user mode.\n"); */
    scheduler.switch_to_user_mode();

    syscall_exec("test");
  } else {
    // The idle task code. Reschedule forever and let
    // the cpu sleep between interrupts.
    for(;;) {
      scheduler.cleanup();
      scheduler.switch_task();
      cpu::halt();
    }
  }

  /* int pid = syscall_getpid(); */
  /* syscall_monitor_write("hello from a task\n"); */
  /* syscall_monitor_write("hello from pid: "); */
  /* syscall_monitor_write_dec(pid); */
  /* syscall_monitor_write("\n"); */
  // syscall_pause();

  /* syscall_monitor_write("done!\n"); */
}

}
