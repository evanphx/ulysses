//
// isr.hpp -- Interface and structures for high level interrupt service routines.
//          Part of this code is modified from Bran's kernel development tutorials.
//          Rewritten for JamesM's kernel development tutorials.
//

#ifndef ISR_H
#define ISR_H

#include "common.hpp"

// A few defines to make life a little easier
#define IRQ0 32
#define IRQ1 33
#define IRQ2 34
#define IRQ3 35
#define IRQ4 36
#define IRQ5 37
#define IRQ6 38
#define IRQ7 39
#define IRQ8 40
#define IRQ9 41
#define IRQ10 42
#define IRQ11 43
#define IRQ12 44
#define IRQ13 45
#define IRQ14 46
#define IRQ15 47

struct Registers {
  u32 gs;
  u32 fs;
  u32 es;
  u32 ds;                  // Data segment selector
  u32 edi, esi, ebp, esp, ebx, edx, ecx, eax; // Pushed by pusha.
  u32 int_no, err_code;    // Interrupt number and error code (if applicable)
  u32 eip, cs, eflags, useresp, ss; // Pushed by the processor automatically.
};

namespace interrupt {

  // Enables registration of callbacks for interrupts or IRQs.
  // For IRQs, to ease confusion, use the #defines above as the
  // first parameter.

  class Handler {
  public:
    virtual void handle(Registers* regs);
  };

  void init();

  void register_isr(u8 nun, Handler* handler);
  void register_interrupt(u8 num, Handler* handler);
}

#endif
