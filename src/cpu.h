namespace cpu {
  static int interrupts_on = 0;

  static inline int enable_interrupts() {
    asm volatile("sti");
  }

  static inline int disable_interrupts() {
    if(!interrupts_on) return 0;
    asm volatile("cli");
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
}
