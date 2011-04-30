#include "monitor.h"
#include "common.h"
#include "rtc.h"

#include "cpu.h"

#define CLKNUM          1193180
/*
 * Enable or disable timer 2.
 * Port 0x61 controls timer 2:
 *   bit 0 gates the clock,
 *   bit 1 gates output to speaker.
 */
inline static void
enable_PIT2(void) {
  asm volatile(
      " inb   $0x61,%%al      \n\t"
      " and   $0xFC,%%al       \n\t"
      " or    $1,%%al         \n\t"
      " outb  %%al,$0x61      \n\t"
      : : : "%al" );
}

inline static void
disable_PIT2(void) {
  asm volatile(
      " inb   $0x61,%%al      \n\t"
      " and   $0xFC,%%al      \n\t"
      " outb  %%al,$0x61      \n\t"
      : : : "%al" );
}

inline static void
set_PIT2(int value) {
  /*
   * First, tell the clock we are going to write 16 bits to the counter
   *   and enable one-shot mode (command 0xB8 to port 0x43)
   * Then write the two bytes into the PIT2 clock register (port 0x42).
   * Loop until the value is "realized" in the clock,
   * this happens on the next tick.
   */
  asm volatile(
      " movb  $0xB8,%%al      \n\t"
      " outb        %%al,$0x43        \n\t"
      " movb        %%dl,%%al        \n\t"
      " outb        %%al,$0x42        \n\t"
      " movb        %%dh,%%al        \n\t"
      " outb        %%al,$0x42        \n"
      "1:          inb        $0x42,%%al        \n\t" 
      " inb        $0x42,%%al        \n\t"
      " cmp        %%al,%%dh        \n\t"
      " jne        1b"
      : : "d"(value) : "%al");
}

inline static u64
get_PIT2(unsigned int *value) {
  register u64        result;
  /*
   * This routine first latches the time (command 0x80 to port 0x43),
   * then gets the time stamp so we know how long the read will take later.
   * Read (from port 0x42) and return the current value of the timer.
   */
  asm volatile(
      " xorl       %%ecx, %%ecx      \n\t"
      " movb       $0x80, %%al       \n\t"
      " outb       %%al, $0x43       \n\t"
      " rdtsc                        \n\t"
      " pushl      %%eax           \n\t"
      " inb        $0x42, %%al        \n\t"
      " movb       %%al, %%cl        \n\t"
      " inb        $0x42, %%al        \n\t"
      " movb       %%al, %%ch        \n\t"
      " popl       %%eax        "
      : "=A"(result), "=c"(*value));
  return result;
}

/*
 * pit_timeRDTSC()
 * This routine sets up PIT counter 2 to count down 1/20 of a second.
 * It pauses until the value is latched in the counter
 * and then reads the time stamp counter to return to the caller.
 */
u64 pit_timeRDTSC(u64* res) {
  int attempts = 0;
  u64 latchTime;
  u64 saveTime, intermediate;
  unsigned int timerValue, lastValue;
  /*
   * Table of correction factors to account for
   *   - timer counter quantization errors, and
   *   - undercounts 0..5
   */
#define SAMPLE_CLKS_EXACT        (((double) CLKNUM) / 20.0)
#define SAMPLE_CLKS_INT          ((int) CLKNUM / 20)
#define SAMPLE_NSECS             (2000000000LL)
#define SAMPLE_MULTIPLIER        (((double)SAMPLE_NSECS)*SAMPLE_CLKS_EXACT)
#define ROUND64(x)               ((u64)((x) + 0.5))

  u64 scale[6] = {
    ROUND64(SAMPLE_MULTIPLIER/(double)(SAMPLE_CLKS_INT-0)), 
    ROUND64(SAMPLE_MULTIPLIER/(double)(SAMPLE_CLKS_INT-1)), 
    ROUND64(SAMPLE_MULTIPLIER/(double)(SAMPLE_CLKS_INT-2)), 
    ROUND64(SAMPLE_MULTIPLIER/(double)(SAMPLE_CLKS_INT-3)), 
    ROUND64(SAMPLE_MULTIPLIER/(double)(SAMPLE_CLKS_INT-4)), 
    ROUND64(SAMPLE_MULTIPLIER/(double)(SAMPLE_CLKS_INT-5))
  };

  int ints = cpu::disable_interrupts();

restart:
  if(attempts >= 2) PANIC("timeRDTSC() calibation failed\n");
  attempts++;

  enable_PIT2();      // turn on PIT2
  set_PIT2(0);        // reset timer 2 to be zero
  latchTime = rdtsc();        // get the time stamp to time 
  latchTime = get_PIT2(&timerValue) - latchTime; // time how long this takes

  set_PIT2(SAMPLE_CLKS_INT);        // set up the timer for (almost) 1/20th a second
  saveTime = rdtsc();        // now time how long a 20th a second is...

  get_PIT2(&lastValue);
  get_PIT2(&lastValue);        // read twice, first value may be unreliable

  do {
    intermediate = get_PIT2(&timerValue);
    if(timerValue > lastValue) {
      console.printf("Hey we are going backwards! %u -> %u, restarting timing\n",
              timerValue, lastValue);
      set_PIT2(0);
      disable_PIT2();
      goto restart;
    }
    lastValue = timerValue;
  } while(timerValue > 5);

  intermediate -= saveTime;                // raw count for about 1/20 second
  intermediate *= scale[timerValue];        // rescale measured time spent
  intermediate /= SAMPLE_NSECS;        // so its exactly 1/20 a second
  intermediate += latchTime;                // add on our save fudge

  set_PIT2(0);                        // reset timer 2 to be zero
  disable_PIT2();                     // turn off PIT 2

  cpu::restore_interrupts(ints);

  intermediate = intermediate * 20;

  *res = intermediate;
  return intermediate;
}
