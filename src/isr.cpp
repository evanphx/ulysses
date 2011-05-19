//
// isr.c -- High level interrupt service routines and interrupt request handlers.
//          Part of this code is modified from Bran's kernel development tutorials.
//          Rewritten for JamesM's kernel development tutorials.
//

#include "common.hpp"
#include "isr.hpp"
#include "monitor.hpp"

namespace interrupt {
  Handler* handlers[256];

  void init() {
    // Nullify all the interrupt handlers.
    memset((u8*)&handlers, 0, sizeof(Handler*)*256);
  }

  void register_interrupt(u8 n, Handler* handler) {
    u8 isr = IRQ0 + n;
    handlers[isr] = handler;
  }

  void register_isr(u8 n, Handler* handler) {
    handlers[n] = handler;
  }

  void Handler::handle(Registers* regs) {
    console.printf("corrupt handler!\n");
    kabort();
  }
}

extern "C" {

  // This gets called from our ASM interrupt handler stub.
  void isr_handler(Registers regs) {
    // This line is important. When the processor extends the 8-bit interrupt number
    // to a 32bit value, it sign-extends, not zero extends. So if the most significant
    // bit (0x80) is set, regs.int_no will be very large (about 0xffffff80).
    u8 int_no = regs.int_no & 0xFF;
    if(interrupt::handlers[int_no] != 0) {
      interrupt::Handler* handler = interrupt::handlers[int_no];
      handler->handle(&regs);
    } else {
      console.printf("unhandled interrupt: %d\n", int_no);

      for(;;) {
        asm volatile("hlt;");
      }
    }
  }

  // This gets called from our ASM interrupt handler stub.
  void irq_handler(Registers regs) {
    // Send an EOI (end of interrupt) signal to the PICs.
    // If this interrupt involved the slave.
    if(regs.int_no >= 40) {
      // Send reset signal to slave.
      outb(0xA0, 0x20);
    }

    // Send reset signal to master. (As well as slave, if necessary).
    outb(0x20, 0x20);

    if(interrupt::handlers[regs.int_no] != 0) {
      interrupt::Handler* handler = interrupt::handlers[regs.int_no];
      handler->handle(&regs);
    } else {
      console.printf("unhandled interrupt: %d\n", regs.int_no - IRQ0);
    }

  }

}
