#include "thread.hpp"

#include "paging.hpp"
#include "kheap.hpp"
#include "descriptor_tables.hpp"
#include "cpu.hpp"
#include "console.hpp"
#include "timer.hpp"
#include "fs.hpp"
#include "fs/devfs.hpp"

Thread::Thread(Process* process)
  : process_(process)
  , alarm_at(0)
  , exit_code(0)
{}

void Thread::sleep_til(int secs) {
  alarm_at = timer.ticks + timer.secs_to_ticks(secs);
}

bool Thread::alarm_expired() {
  return state == Thread::eWaiting && 
         alarm_at != 0 &&
         alarm_at <= timer.ticks;
}
