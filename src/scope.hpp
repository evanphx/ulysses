#ifndef SCOPE_HPP
#define SCOPE_HPP

#include "cpu.hpp"

class SuspendInterrupts {
  SuspendInterrupts() {
    cpu::disable_interrupts();
  }

  ~SuspendInterrupts() {
    cpu::enable_interrupts();
  }
};

#endif
