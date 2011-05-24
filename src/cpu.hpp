#ifndef CPU_HPP
#define CPU_HPP

namespace cpu {

  extern "C" u32 read_flags();

  static const unsigned int cPageSize = 0x1000;
  static const unsigned int cPageMask = ~0xfff;
  static const unsigned int cMaxAddress = 0xFFFFFFFF;

  static inline u32 page_align(u32 addr) {
    return (addr + cPageSize - 1) & cPageMask;
  }

  static inline int enable_interrupts() {
    asm volatile("sti");
    return 1;
  }

  static inline int disable_interrupts() {
    asm volatile("cli");
    return 0;
  }

  enum Flags {
    eIF = 1 << 9
  };

  static inline bool interrupts_enabled_p() {
    u32 flags = read_flags();
    return (flags & eIF) == eIF;
  }

  static inline void restore_interrupts(int val) {
    if(val) {
      disable_interrupts();
    } else {
      enable_interrupts();
    }
  }

  static inline void halt() {
    asm volatile("hlt;");
  }

  static inline void halt_loop() {
    for(;;) halt();
  }

  static inline void jump_to(u32 address) {
    asm volatile("jmp *%0" :: "r"(address));
  }

  static inline u32 page_directory() {
    u32 pd;
    asm volatile("mov %%cr3, %0" : "=r" (pd));
    return pd;
  }

  static inline void set_page_directory(u32 addr) {
    asm volatile("mov %0, %%cr3" : : "r" (addr));
  }

  static inline void enable_paging() {
    u32 cr0;
    asm volatile("mov %%cr0, %0": "=r"(cr0));
    cr0 |= 0x80000000; // Enable paging!
    asm volatile("mov %0, %%cr0":: "r"(cr0));
  }

  static inline u32 fault_address() {
    u32 addr;
    asm volatile("mov %%cr2, %0" : "=r" (addr));
    return addr;
  }

  static inline void flush_tbl() {
    // Flush the TLB by reading and writing the page directory address again.
    set_page_directory(page_directory());
  }

}

#endif
