// timer.c -- Initialises the PIT, and handles clock updates.
//            Written for JamesM's kernel development tutorials.

#include "timer.hpp"
#include "isr.hpp"
#include "monitor.hpp"
#include "rtc.hpp"
#include "scheduler.hpp"

Timer timer = {0};

class TimerCallback : public interrupt::Handler {
public:
  void handle(Registers* regs) {
    timer.ticks++;
    update_clock();

    scheduler.on_tick();
    // scheduler.switch_task();
  }
};

void Timer::init(u32 frequency) {
  ticks = 0;

  init_clock();

  static TimerCallback callback;

  // Firstly, register our timer callback.
  interrupt::register_interrupt(0, &callback);

  // The value we send to the PIT is the value to divide it's input clock
  // (1193180 Hz) by, to get our required frequency. Important to note is
  // that the divisor must be small enough to fit into 16-bits.
  u32 divisor = 1193180 / frequency;

  // Send the command byte.
  outb(0x43, 0x36);

  // Divisor has to be sent byte-wise, so split here into upper/lower bytes.
  u8int l = (u8int)(divisor & 0xFF);
  u8int h = (u8int)((divisor>>8) & 0xFF);

  // Send the frequency divisor.
  outb(0x40, l);
  outb(0x40, h);
}
