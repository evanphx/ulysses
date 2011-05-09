namespace cpu {

  static const int cPageSize = 0x1000;

  static int interrupts_on = 0;

  static inline int enable_interrupts() {
    asm volatile("sti");
    return 1;
  }

  static inline int disable_interrupts() {
    if(!interrupts_on) return 0;
    asm volatile("cli");
    return 1;
  }

  static inline void restore_interrupts(int val) {
    if(val) {
      interrupts_on = 1;
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
