#include "monitor.hpp"
#include "timer.hpp"
#include "rtc.hpp"
#include "pit.hpp"

#define SEC 0
#define MIN 2
#define HOUR 4
#define DOM 7
#define MON 8
#define YEAR 9

u8int cmos_read(int addr) {
  outb(0x70, addr);
  return inb(0x71);
}

unsigned int cur_sec;
unsigned int cur_usec;

u64 cycle_per_sec;
u64 last_rdtsc = 0;

#define BCD_TO_BIN(val) ((val)=((val)&15) + ((val)>>4)*10)

void init_clock() {
	unsigned int year, mon, day, hour, min, sec;

  do {
    sec = cmos_read(SEC);
    min = cmos_read(MIN);
    hour = cmos_read(HOUR);
    day = cmos_read(DOM);
    mon = cmos_read(MON);
    year = cmos_read(YEAR);
  } while(sec != cmos_read(SEC));

  /* year += 1900; */


  BCD_TO_BIN(sec);
  BCD_TO_BIN(min);
  BCD_TO_BIN(hour);
  BCD_TO_BIN(day);
  BCD_TO_BIN(mon);
  BCD_TO_BIN(year);

  if(year < 30) {
    year += 2000;
  } else {
    year += 1970;
  }

  if(0 >= (int)(mon -= 2)) {
    mon += 12;
    year -= 1;
  }

  unsigned int year_day = 
    (year/4 - year/100 + year/400 + 367*mon/12 + day) + year*365 - 719499;

  if(year_day < 0) {
    kputs("bad year_day!\n");
  }

  unsigned int epoch = sec + 60 * (min + 60 * (hour + 24 * year_day));

  console.printf("Time: %2d:%2d:%2d %4d/%2d/%2d (%d)\n",
                 hour, min, sec,
                 year, mon, day,
                 epoch);

  cur_sec = epoch;
  cur_usec = 0;

  /*
  // Do it 10 times to let it warm up so we get a decent one
  int i = 0;
  while(i++ < 10) {
    pit_timeRDTSC(&cycle_per_sec);
  }

  console.printf("rdtsc cycle/sec: %llu\n", cycle_per_sec);

  last_rdtsc = rdtsc();
  */
  
  /* u64 tsc20 = pit_timeRDTSC(); */
  /* console.printf("tsc20: %lld\n", tsc20); */
}

void update_clock() {
  cur_usec += SLICE_US;
  /*
  if(cur_usec > 1000000) {
    u64 cur_rdtsc = rdtsc();

    cur_usec -= 1000000;
    cur_sec++;
    kputs("time: ");
    console.write_dec(cur_sec);
    console.printf(".%d\n", cur_usec);

    u64 diff_tsc = cur_rdtsc - last_rdtsc;
    console.printf("diff_tsc: %lld, ", diff_tsc);
    if(diff_tsc > cycle_per_sec) {
      console.printf("against cps: %lld\n", diff_tsc - cycle_per_sec);
    } else {
      console.printf("against cps: %lld\n", cycle_per_sec - diff_tsc);
    }
    last_rdtsc = cur_rdtsc;
  }
  */
}
