#include "keyboard.h"
#include "isr.h"
#include "monitor.h"

#include "keyboard.h"

Keyboard keyboard = {{0x60}};

static void keyboard_callback(registers_t* regs) {
  keyboard.pressed();
}

void Keyboard::pressed() {
  unsigned char code = io.inb();

  if(code & 0x80) {
    // handle press
  } else {
    console.printf("keypress: %d\n", code);
  }
}

void Keyboard::init() {
  register_interrupt_handler(1, &keyboard_callback);
}
