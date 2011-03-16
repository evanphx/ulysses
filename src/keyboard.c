#include "keyboard.h"
#include "isr.h"
#include "monitor.h"

static void keyboard_callback(registers_t* regs) {
  unsigned char code = inb(0x60);

  if(code & 0x80) {
    // handle press
  } else {
    monitor_write("keypress: ");
    monitor_write_dec(code);
    monitor_write("\n");
  }
}

void init_keyboard() {
  register_interrupt_handler(IRQ1, &keyboard_callback);
}
