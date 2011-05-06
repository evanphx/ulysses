#include "io.hpp"
#include "common.hpp"

struct RTL8139 {
  IOPort ctrl;
  IOPort status;

  int irq;
  u32 rx_buffer;
  int cur_rx;

  struct tx_buffer {
    u32 virt;
    u32 phys;
  };

  struct tx_buffer tx_buffers[4];

  int tx_desc;
  char mac[6];

  void pm_wakeup();
  void reset();

  void power_on();
  u16 ack();
  void read_mac();

  void enable_tx_rx();

  void set_config();
  void unlock();
  void lock();
  void set_clock(int run);
  void set_rx_buffer(u32 addr);

  void reset_rx_stats();
  void enable_interrupts();

  u32 last_tx_status();
  void receive(u16 status);
  void transmit(u8* buf, int size);

  void init();
};

void init_rtl8139(u32int io_port, u8int irq);
void xmit_packet(u8int* buf, int size);
char* eth_mac();
