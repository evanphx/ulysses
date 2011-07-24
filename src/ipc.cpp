#include "common.hpp"
#include "ipc.hpp"

namespace ipc {
  int channel_create() {
    Channel* chan = new(kheap) Channel;
    return scheduler->current->add_channel(chan);
  }

  int channel_attach(int pid, int cid) {
    Thread* task = scheduler->find_task(pid);
    if(!task) return -1;

    Channel* chan = task->channel(cid);
    if(!chan) return -1;

    return scheduler->current->add_channel(chan);
  }
}
