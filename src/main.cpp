// main.c -- Defines the C-code kernel entry point, calls initialisation routines.
//           Made for JamesM's tutorials <www.jamesmolloy.co.uk>

#include "monitor.hpp"
#include "descriptor_tables.hpp"
#include "timer.hpp"
#include "paging.hpp"
#include "multiboot.hpp"
#include "fs.hpp"
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
#include "scheduler.hpp"
#include "tar.hpp"
#include "inspector.hpp"

#include "cpu.hpp"

const char* init_path = "/bin/init";

const char* init_argv[] = { init_path, 0};
// const char* init_argv[] = { "/dash", 0 };
const char* init_envp[] = { "TERM=ulysses", "OS=ulysses", 0 };

void run_init() {
  syscall_mount("/dev", "devfs", 0);

  int i = syscall_open("/dev/console", 0);
  syscall_dup(i);
  syscall_dup(i);

  syscall_exec(init_path, init_argv, init_envp);
}

static void process_cmdline(char* cmdline) {
  char* pos = cmdline;
  while(*pos) {
    if(*pos == ' ') {
      init_path = pos+1;
      return;
    }

    pos++;
  }
}

extern "C" int kmain(struct multiboot *mboot_ptr, u32 magic, u32 kstart, u32 kend) {
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

  char* cmdline = (char*)(mboot_ptr->cmdline + KERNEL_VIRTUAL_BASE);

  console.printf("Kernel %x-%x : %d\n", kstart, kend, kernel_size);
  console.printf("Command Line: %s\n", cmdline);

  process_cmdline(cmdline);

  // Initialise the PIT to 100Hz
  cpu::enable_interrupts();

  timer.init(SLICE_HZ);

  cpu::print_cpuid();

  // Find the location of our initial ramdisk.
  ASSERT(mboot_ptr->mods_count > 0);
  u32 initrd_location = *((u32*)mboot_ptr->mods_addr);
  initrd_location += KERNEL_VIRTUAL_BASE;

  u32 initrd_end = *(u32*)(mboot_ptr->mods_addr+4);
  initrd_end += KERNEL_VIRTUAL_BASE;

  console.printf("initrd: 0x%x - 0x%x (%d)\n", 
      initrd_location, initrd_end-1,
      initrd_end - initrd_location);

  // Start paging.
  vmem.init(mem_total, kstart, kend, initrd_end);

  inspector.init(mboot_ptr);

  // Start multithreading.
  scheduler.init();

  keyboard.init();

  initialise_syscalls();

  fs::registry.init();
  devfs::main.init();
  ext2::init();
  tmpfs::init();

  tmpfs::FS* fs = new (kheap) tmpfs::FS();
  fs_root = fs->root();

  tar::Archive arc((u8*)initrd_location, initrd_end - initrd_location);
  int count = arc.load_into(fs->specific_root());

  console.printf("Imported entries from tar: %d\n", count);

  // Initialise the initial ramdisk, and set it as the filesystem root.
  // fs_root = initrd::fs.init(initrd_location);

  block::registry.init();
  console_driver::init();

  pci_bus.init();

  // block::registry.print();

  scheduler.spawn_init(run_init);

  // The idle thread code. Reschedule forever and let
  // the cpu sleep between interrupts.
  for(;;) {
    scheduler.cleanup();
    scheduler.switch_thread();
    cpu::enable_interrupts();
    cpu::halt();
  }

  console.printf("BUG! BUG BUG BUG BUG!\n");

  return 0;
}
