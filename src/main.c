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

extern u32int placement_address;
u32int initial_esp;
u32int initrd_location;

void main2();

static void show_cpuid() {
  u32int ebx, edx, ecx;
  u32int code = 0;

  asm volatile("cpuid" : "=b"(ebx),"=d"(edx),"=c"(ecx) : "a"(code));

  monitor_write("CPUID: ");
  monitor_put((ebx & 0x000000ff) >> 0);
  monitor_put((ebx & 0x0000ff00) >> 8);
  monitor_put((ebx & 0x00ff0000) >> 16);
  monitor_put((ebx & 0xff000000) >> 24);
  monitor_put((edx & 0x000000ff) >> 0);
  monitor_put((edx & 0x0000ff00) >> 8);
  monitor_put((edx & 0x00ff0000) >> 16);
  monitor_put((edx & 0xff000000) >> 24);
  monitor_put((ecx & 0x000000ff) >> 0);
  monitor_put((ecx & 0x0000ff00) >> 8);
  monitor_put((ecx & 0x00ff0000) >> 16);
  monitor_put((ecx & 0xff000000) >> 24);
  monitor_write("\n");
}

/* static inline void rdtsc(dword *upper, dword *lower) { */
  /* asm volatile("rdtsc\n" : "=a"(*lower), "=d"(*upper)); */
/* } */

int main(struct multiboot *mboot_ptr, u32int initial_stack)
{
    initial_esp = initial_stack;
    // Initialise all the ISRs and segmentation
    init_descriptor_tables();
    // Initialise the screen (by clearing it)
    // monitor_clear();
    monitor_setup();

    // Initialise the PIT to 100Hz
    asm volatile("sti");
    init_timer(SLICE_HZ);

    show_cpuid();

    // Find the location of our initial ramdisk.
    ASSERT(mboot_ptr->mods_count > 0);
    initrd_location = *((u32int*)mboot_ptr->mods_addr);
    u32int initrd_end = *(u32int*)(mboot_ptr->mods_addr+4);
    // Don't trample our module with placement accesses, please!
    placement_address = initrd_end;

    monitor_write("initrd: ");
    monitor_write_hex(initrd_location);
    monitor_write("-");
    monitor_write_hex(initrd_end-1);
    // Start paging.
    initialise_paging();

    // Start multitasking.
    initialise_tasking();

    u32int sp = 0xE0000000;

    asm volatile(
      "mov %0, %%esp\n"
      "mov %0, %%ebp\n"
      "jmp main2\n"
    : : "r" (sp));
}

void main2() {
    // Initialise the initial ramdisk, and set it as the filesystem root.
    fs_root = initialise_initrd(initrd_location);

    init_keyboard();
    initialise_syscalls();

    init_pci();

    /* monitor_write("Switching to user mode.\n"); */
    /* switch_to_user_mode(); */

    /* syscall_monitor_write("Hello, user world!\n"); */

    for(;;) {
      asm volatile("hlt;");
    }
}
