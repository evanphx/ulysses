#ifndef IPC_H
#define IPC_H

namespace ipc {

  class Channel {
    
  };

  int channel_create();
  int channel_connect(int pid, int cid);
  int msg_recv(int cid, void* msg, int len);
}

#endif
