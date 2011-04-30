#include "keyboard.h"
#include "isr.h"
#include "monitor.h"

static void keyboard_callback(registers_t* regs) {
  unsigned char code = inb(0x60);

  if(code & 0x80) {
    // handle press
  } else {
    console.printf("keypress: %d\n", code);
  }
}

void init_keyboard() {
  register_interrupt_handler(1, &keyboard_callback);
}
