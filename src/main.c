// main.c -- Defines the C-code kernel entry point, calls initialisation routines.
//           Made for JamesM's tutorials <www.jamesmolloy.co.uk>

#include "monitor.h"
#include "descriptor_tables.h"
#include "timer.h"
#include "paging.h"
#include "multiboot.h"
#include "fs.h"
#include "initrd.h"
#include "task.h"
#include "syscall.h"
#include "keyboard.h"
#include "pci.h"
#include "rtc.h"

#include "cpu.h"

extern "C" {

extern u32int placement_address;
u32int initial_esp;
u32int initrd_location;

void kmain2();

static void show_cpuid() {
  u32int ebx, edx, ecx;
  u32int code = 0;

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

int kmain(struct multiboot *mboot_ptr, u32int initial_stack)
{
    initial_esp = initial_stack;
    // Initialise all the ISRs and segmentation
    init_descriptor_tables();
    // Initialise the screen (by clearing it)
    // console.clear();
    console.setup();

    // Initialise the PIT to 100Hz
    cpu::interrupts_on = 1;
    cpu::enable_interrupts();

    init_timer(SLICE_HZ);

    show_cpuid();

    // Find the location of our initial ramdisk.
    ASSERT(mboot_ptr->mods_count > 0);
    initrd_location = *((u32int*)mboot_ptr->mods_addr);
    u32int initrd_end = *(u32int*)(mboot_ptr->mods_addr+4);
    // Don't trample our module with placement accesses, please!
    placement_address = initrd_end;

    console.printf("initrd: 0x%x - 0x%x\n", initrd_location, initrd_end-1);

    // Start paging.
    vmem.init();

    // Start multitasking.
    scheduler.init();

    u32int sp = 0xE0000000;

    asm volatile(
      "mov %0, %%esp\n"
      "mov %0, %%ebp\n"
      "jmp kmain2\n"
    : : "r" (sp));
}

void kmain2() {
  // Initialise the initial ramdisk, and set it as the filesystem root.
  fs_root = initrd::fs.init(initrd_location);

  console.printf("Initrd entries: %d\n", initrd::fs.nroot_nodes);

  keyboard.init();

  initialise_syscalls();

  pci_bus.init();

  //console.write("Switching to user mode.\n");
  // scheduler.switch_to_user_mode();

  // syscall_monitor_write("Hello, user world!\n");

  cpu::halt_loop();
}

}
