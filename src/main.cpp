// main.c -- Defines the C-code kernel entry point, calls initialisation routines.
//           Made for JamesM's tutorials <www.jamesmolloy.co.uk>

#include "monitor.hpp"
#include "descriptor_tables.hpp"
#include "timer.hpp"
#include "paging.hpp"
#include "multiboot.hpp"
#include "fs.hpp"
#include "task.hpp"
#include "syscall.hpp"
#include "keyboard.hpp"
#include "pci.hpp"
#include "rtc.hpp"
#include "elf.hpp"
#include "block.hpp"
#include "fs/initrd.hpp"
#include "fs/devfs.hpp"
#include "fs/ext2.hpp"
#include "fs/tmpfs.hpp"
#include "character/console.hpp"

#include "cpu.hpp"

extern "C" {

u32 initrd_location;

const char* init_argv[] = { "/bin/init", "start", 0 };
const char* init_envp[] = { "TERM=ulysses", "OS=ulysses", 0 };

void run_init() {
  syscall_exec("test", init_argv, init_envp);
}

int kmain(struct multiboot *mboot_ptr, u32 magic, u32 kstart, u32 kend) {
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

  u32 kernel_size = kend - kstart;

  console.printf("Kernel %x-%x : %d\n", kstart, kend, kernel_size);

  // Initialise the PIT to 100Hz
  cpu::enable_interrupts();

  timer.init(SLICE_HZ);

  cpu::print_cpuid();

  // Find the location of our initial ramdisk.
  ASSERT(mboot_ptr->mods_count > 0);
  initrd_location = *((u32*)mboot_ptr->mods_addr);
  initrd_location += KERNEL_VIRTUAL_BASE;

  u32 initrd_end = *(u32*)(mboot_ptr->mods_addr+4);
  initrd_end += KERNEL_VIRTUAL_BASE;

  console.printf("initrd: 0x%x - 0x%x\n", initrd_location, initrd_end-1);

  // Start paging.
  vmem.init(mem_total, kstart, kend, initrd_end);

  // Start multitasking.
  scheduler.init();

  // Initialise the initial ramdisk, and set it as the filesystem root.
  fs_root = initrd::fs.init(initrd_location);

  keyboard.init();

  initialise_syscalls();

  fs::registry.init();
  devfs::main.init();
  ext2::init();
  tmpfs::init();

  block::registry.init();
  console_driver::init();

  pci_bus.init();

  // block::registry.print();

  scheduler.spawn_thread(run_init);

  // The idle task code. Reschedule forever and let
  // the cpu sleep between interrupts.
  for(;;) {
    scheduler.cleanup();
    scheduler.switch_task();
    cpu::enable_interrupts();
    cpu::halt();
  }

  return 0;
}

}
