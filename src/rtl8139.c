#include "monitor.h"
#include "isr.h"
#include "eth.h"
#include "kheap.h"

#define IN_KERNEL

#include "lwip/netif.h"
#include "ipv4/lwip/ip.h"
#include "netif/etharp.h"

/* Symbolic offsets to registers. */
enum RTL8139_registers {
    MAC0 = 0,        /* Ethernet hardware address. */
    MAR0 = 8,        /* Multicast filter. */
    TxStatus0 = 0x10,/* Transmit status (Four 32bit registers). C mode only */
                     /* Dump Tally Conter control register(64bit). C+ mode only */
    TxAddr0 = 0x20,  /* Tx descriptors (also four 32bit). */
    RxBuf = 0x30,
    ChipCmd = 0x37,
    RxBufPtr = 0x38,
    RxBufAddr = 0x3A,
    IntrMask = 0x3C,
    IntrStatus = 0x3E,
    TxConfig = 0x40,
    RxConfig = 0x44,
    Timer = 0x48,        /* A general-purpose counter. */
    RxMissed = 0x4C,    /* 24 bits valid, write clears. */
    Cfg9346 = 0x50,
    Config0 = 0x51,
    Config1 = 0x52,
    FlashReg = 0x54,
    MediaStatus = 0x58,
    Config3 = 0x59,
    Config4 = 0x5A,        /* absent on RTL-8139A */
    HltClk = 0x5B,
    MultiIntr = 0x5C,
    PCIRevisionID = 0x5E,
    TxSummary = 0x60, /* TSAD register. Transmit Status of All Descriptors*/
    BasicModeCtrl = 0x62,
    BasicModeStatus = 0x64,
    NWayAdvert = 0x66,
    NWayLPAR = 0x68,
    NWayExpansion = 0x6A,
    /* Undocumented registers, but required for proper operation. */
    FIFOTMS = 0x70,        /* FIFO Control and test. */
    CSCR = 0x74,        /* Chip Status and Configuration Register. */
    PARA78 = 0x78,
    PARA7c = 0x7c,        /* Magic transceiver parameter register. */
    Config5 = 0xD8,        /* absent on RTL-8139A */
    /* C+ mode */
    TxPoll        = 0xD9,    /* Tell chip to check Tx descriptors for work */
    RxMaxSize    = 0xDA, /* Max size of an Rx packet (8169 only) */
    CpCmd        = 0xE0, /* C+ Command register (C+ mode only) */
    IntrMitigate    = 0xE2,    /* rx/tx interrupt mitigation control */
    RxRingAddrLO    = 0xE4, /* 64-bit start addr of Rx ring */
    RxRingAddrHI    = 0xE8, /* 64-bit start addr of Rx ring */
    TxThresh    = 0xEC, /* Early Tx threshold */
};

enum ChipCmdBits {
    CmdReset = 0x10,
    CmdRxEnb = 0x08,
    CmdTxEnb = 0x04,
    RxBufEmpty = 0x01,
};

enum Cfg9346Bits {
    Cfg9346_Lock = 0x00,
    Cfg9346_Unlock = 0xC0,
};

/* Bits in Config1 */
enum Config1Bits {
    Cfg1_PM_Enable = 0x01,
    Cfg1_VPD_Enable = 0x02,
    Cfg1_PIO = 0x04,
    Cfg1_MMIO = 0x08,
    LWAKE = 0x10,        /* not on 8139, 8139A */
    Cfg1_Driver_Load = 0x20,
    Cfg1_LED0 = 0x40,
    Cfg1_LED1 = 0x80,
    SLEEP = (1 << 1),    /* only on 8139, 8139A */
    PWRDN = (1 << 0),    /* only on 8139, 8139A */
};

/* Bits in RxConfig. */
enum rx_mode_bits {
    AcceptErr = 0x20,
    AcceptRunt = 0x10,
    AcceptBroadcast = 0x08,
    AcceptMulticast = 0x04,
    AcceptMyPhys = 0x02,
    AcceptAllPhys = 0x01,
};

enum RxConfigBits {
    /* rx fifo threshold */
    RxCfgFIFOShift = 13,
    RxCfgFIFONone = (7 << RxCfgFIFOShift),

    /* Max DMA burst */
    RxCfgDMAShift = 8,
    RxCfgDMAUnlimited = (7 << RxCfgDMAShift),

    /* rx ring buffer length */
    RxCfgRcv8K = 0,
    RxCfgRcv16K = (1 << 11),
    RxCfgRcv32K = (1 << 12),
    RxCfgRcv64K = (1 << 11) | (1 << 12),

    /* Disable packet wrap at end of Rx buffer. (not possible with 64k) */
    RxNoWrap = (1 << 7),
};

enum TxStatusBits {
    TxHostOwns = 0x2000,
    TxUnderrun = 0x4000,
    TxStatOK = 0x8000,
    TxOutOfWindow = 0x20000000,
    TxAborted = 0x40000000,
    TxCarrierLost = 0x80000000,
};

/* Bits in TxConfig. */
enum tx_config_bits {

        /* Interframe Gap Time. Only TxIFG96 doesn't violate IEEE 802.3 */
        TxIFGShift = 24,
        TxIFG84 = (0 << TxIFGShift),    /* 8.4us / 840ns (10 / 100Mbps) */
        TxIFG88 = (1 << TxIFGShift),    /* 8.8us / 880ns (10 / 100Mbps) */
        TxIFG92 = (2 << TxIFGShift),    /* 9.2us / 920ns (10 / 100Mbps) */
        TxIFG96 = (3 << TxIFGShift),    /* 9.6us / 960ns (10 / 100Mbps) */

    TxLoopBack = (1 << 18) | (1 << 17), /* enable loopback test mode */
    TxCRC = (1 << 16),    /* DISABLE appending CRC to end of Tx packets */
    TxClearAbt = (1 << 0),    /* Clear abort (WO) */
    TxDMAShift = 8,        /* DMA burst value (0-7) is shifted this many bits */
    TxRetryShift = 4,    /* TXRR value (0-15) is shifted this many bits */

    TxVersionMask = 0x7C800000, /* mask out version bits 30-26, 23 */
};

/* Interrupt register bits, using my own meaningful names. */
enum IntrStatusBits {
    PCIErr = 0x8000,
    PCSTimeout = 0x4000,
    RxFIFOOver = 0x40,
    RxUnderrun = 0x20,
    RxOverflow = 0x10,
    TxErr = 0x08,
    TxOK = 0x04,
    RxErr = 0x02,
    RxOK = 0x01,

    RxAckBits = RxFIFOOver | RxOverflow | RxOK,
    AllInterrupts = PCIErr | PCSTimeout
                  | RxFIFOOver | RxUnderrun | RxOverflow | RxErr | RxOK
                  | TxErr | TxOK,
    RxInterrupts = RxUnderrun | RxOverflow | RxFIFOOver | RxErr | RxOK,
    TxInterrupts = TxErr | TxOK

};

enum RxStatusBits {
    RxMulticast = 0x8000,
    RxPhysical = 0x4000,
    RxBroadcast = 0x2000,
    RxBadSymbol = 0x0020,
    RxRunt = 0x0010,
    RxTooLong = 0x0008,
    RxCRCErr = 0x0004,
    RxBadAlign = 0x0002,
    RxStatusOK = 0x0001,

    RxErrors = RxBadSymbol | RxRunt | RxTooLong | RxCRCErr | RxBadAlign
};

#define fifo_size 4
#define dma_burst 4

static u32int default_rx_config =
    RxCfgRcv32K | RxNoWrap |
    AcceptBroadcast | AcceptAllPhys |
   (fifo_size << RxCfgFIFOShift) |
   (dma_burst << RxCfgDMAShift);

static u32int default_tx_config =
   (dma_burst << TxDMAShift) | TxIFG96;

static int default_rx_buf_len = 8192 << 2;

static u32int rtl8139_io;
static u32int rtl8139_irq;
static u32int rtl8139_rx_buffer;
static u32int rtl8139_cur_rx;

struct tx_buffer {
  u32int virt;
  u32int phys;
};

static struct tx_buffer rtl8139_tx_buffers[4];
static u32int rtl8139_tx_desc;

static char rtl8139_mac[6];

static struct netif rtl8139_lwip_netif;
static struct ip_addr rtl8139_ipaddr = {0};
static struct ip_addr rtl8139_netmask = {0};
static struct ip_addr rtl8139_gw = {0};

// END STATE

char* eth_mac() {
  return rtl8139_mac;
}

static void rtl8139_read_mac(u32int io, char* mac) {
  int i;
  for(i = 0; i < 6; i++) {
    mac[i] = inb(io + MAC0 + i);
  }
}

static void rtl8139_power_on(u32int io) {
  outb(io + Config1, 0);
}

static void rtl8139_pm_wakeup(u32int io) {
  u8int config = inb(io + Config1);
  config |= Cfg1_PM_Enable;
  outb(io + Config1, config);
}

static void rtl8139_reset(u32int io) {
  int i = 0;
  outb(io + ChipCmd, CmdReset);

  monitor_write("rtl8139: Resetting...");
  for(;;) {
    u8int byte = inb(io + ChipCmd);
    if((byte & CmdReset) == 0) {
      monitor_write("reset\n");
      return;
    }
  }

  monitor_write("rtl8139: Timeout resetting.\n");
}

static void rtl8139_enable_tx_rx(u32int io) {
  outb(io + ChipCmd, CmdTxEnb | CmdRxEnb);
}

static void rtl8139_set_config(u32int io) {
  outl(io + RxConfig, default_rx_config);
  outl(io + TxConfig, default_tx_config);
}

static void rtl8139_unlock(u32int io) {
  outb(io + Cfg9346, Cfg9346_Unlock);
}

static void rtl8139_lock(u32int io) {
  outb(io + Cfg9346, Cfg9346_Lock);
}

static void rtl8139_set_clock(u32int io, int run) {
  outb(io + HltClk, run ? 'R' : 'H');
}

static void rtl8139_set_rx_buffer(u32int io, u32int addr) {
  outl(io + RxBuf, addr);
}

static void rtl8139_reset_rx_stats(u32int io) {
  outl(io + RxMissed, 0);
}

static void rtl8139_enable_interrupts(u32int io) {
  outw(io + IntrMask, AllInterrupts);
}

static u16int rtl8139_ack(u32int io) {
  u16int status = inw(io + IntrStatus);
  outw(io + IntrStatus, status);
  return status;
}

static u16int rtl8139_status(u32int io) {
  return inw(io + IntrStatus);
}

static u32int rtl8139_last_tx_status(u32int io) {
  int entry = rtl8139_tx_desc;
  if(entry == 0) {
    entry = 3;
  } else {
    entry--;
  }

  return inl(io + TxStatus0 + entry*4);
}

static void rtl8139_receive(u32int io, u16int status) {
  int i;

  while((inb(io + ChipCmd) & RxBufEmpty) == 0) {
    int offset = rtl8139_cur_rx % default_rx_buf_len;

    u16int buf_addr = inw(io + RxBufAddr);
    u16int buf_ptr =  inw(io + RxBufPtr);
    u8int cmd = inb(io + ChipCmd);

    u32int* buf_start = (u32int*)(rtl8139_rx_buffer + offset);
    u8int*  u8buf_start = (u8int*)(rtl8139_rx_buffer + offset);
    u32int rx_status = *buf_start;
    int rx_size = rx_status >> 16;

    monitor_write("rtl8139: receive: buf_addr=");
    monitor_write_hex(buf_addr);
    monitor_write(", buf_ptr=");
    monitor_write_hex(buf_ptr);
    monitor_write(", cmd=");
    monitor_write_hex(cmd);
    monitor_write(", rx_status=");
    monitor_write_hex(rx_status);
    monitor_write(", rx_size=");
    monitor_write_dec(rx_size);
    monitor_write("\n");

    monitor_write("rtl8139: ring buffer=");
    monitor_write_hex((u32int)buf_start);
    monitor_write(" size=");
    monitor_write_dec(rx_size);
    monitor_write(" cur_rx=");
    monitor_write_dec(rtl8139_cur_rx);
    monitor_write(" offset=");
    monitor_write_dec(offset);
    kputs("\n");

    if(rx_status & RxErrors) {
      monitor_write("rtl8139: Detected frame errors.\n");
    } else {
      /*
      monitor_write("Frame:\n");
      for(i = 4; i < rx_size; i++) {
        monitor_write(" ");
        monitor_write_hex_byte(u8buf_start[i]);
      }

      monitor_write("\n");
      */

      struct pbuf* pkt = pbuf_alloc(PBUF_RAW, rx_size, PBUF_RAM);
      pbuf_take(pkt, u8buf_start + 4, rx_size);
      eth_handoff(u8buf_start + 4, rx_size);
      ethernet_input(pkt, &rtl8139_lwip_netif);
    }

    rtl8139_cur_rx = (rtl8139_cur_rx + rx_size + 4 + 3) & ~3;
    outw(io + RxBufPtr, rtl8139_cur_rx - 16);
  }
}

static void rtl8139_interrupt(registers_t* regs) {
  u32int io = rtl8139_io;

  u16int status = rtl8139_ack(io);

  if(status & AllInterrupts == 0) return;
  if(status & RxInterrupts) rtl8139_receive(io, status);
  if(status & TxInterrupts) {
    int tx_status = rtl8139_last_tx_status(io);
    if(status == TxOK) {
      kputs("Got a TxOK interrupt.\n");
    } else {
      kputs("Got a TxErr interrupt.\n");
    }
  }
}

void rtl8139_transmit(u32int io, u8int* buf, int size) {
  int entry = rtl8139_tx_desc;

  kputs("rtl8139: Writing to tx descriptor: ");
  monitor_write_dec(entry);
  kputs("\n");

  kprintf("copy %p (%d) to %p\n", buf, size, rtl8139_tx_buffers[entry].virt);
  memcpy((u8int*)rtl8139_tx_buffers[entry].virt, buf, size);
  outl(io + TxAddr0 + entry*4, rtl8139_tx_buffers[entry].phys);
  outl(io + TxStatus0 + entry*4, size & 0x1fff);

  if(++rtl8139_tx_desc > 4) rtl8139_tx_desc = 0;
}

void xmit_packet(u8int* buf, int size) {
  rtl8139_transmit(rtl8139_io, buf, size);
}

err_t rtl8139_open(struct netif* netif);
err_t rtl8139_input(struct pbuf* p, struct netif* netif) {
  kputs("Would send packet out rtl8139 for lwip.\n");
}

err_t rtl8139_link_output(struct netif* netif, struct pbuf* p) {
  xmit_packet((u8int*)p->payload, p->len);
}

err_t rtl8139_output(struct netif* nif, struct pbuf* p, struct ip_addr* dest) {
  etharp_output(nif, p, dest); 
}

void init_rtl8139(u32int io, u8int irq) {
  rtl8139_io = io;
  rtl8139_irq = irq;

  memset((u8int*)&rtl8139_lwip_netif, 0, sizeof(struct netif));

  rtl8139_ipaddr.addr = inet_addr("10.0.2.15");

  netif_add(&rtl8139_lwip_netif, &rtl8139_ipaddr, &rtl8139_netmask,
            &rtl8139_gw, 0, rtl8139_open, rtl8139_input);

  rtl8139_lwip_netif.linkoutput = rtl8139_link_output;
  rtl8139_lwip_netif.output = rtl8139_output;

  netif_set_default(&rtl8139_lwip_netif);
  netif_set_up(&rtl8139_lwip_netif);
}

err_t rtl8139_open(struct netif* netif) {
  int i;

  u32int io = rtl8139_io;
  u32int irq = rtl8139_irq;

  rtl8139_power_on(io);

  rtl8139_read_mac(io, rtl8139_mac);

  memcpy((u8int*)rtl8139_lwip_netif.hwaddr, (u8int*)rtl8139_mac, 6);
  rtl8139_lwip_netif.hwaddr_len = 6;

  monitor_write("Realtek 8139: MAC ");
  for(i = 0; i < 6; i++) {
    monitor_write_hex_np(rtl8139_mac[i]);
    if(i != 5) monitor_write(":");
  }

  monitor_write("\n");

  rtl8139_unlock(io);
  rtl8139_pm_wakeup(io);
  rtl8139_set_clock(io, 0);

  rtl8139_reset(io);
  rtl8139_enable_tx_rx(io);
  rtl8139_set_config(io);

  u32int phys_rx_ring = 0;
  u32int virt_rx_ring = kmalloc_ap(default_rx_buf_len + 16, &phys_rx_ring);

  monitor_write("rtl8139: phys=");
  monitor_write_hex(phys_rx_ring);
  monitor_write(" virt=");
  monitor_write_hex(virt_rx_ring);
  monitor_write("\n");

  rtl8139_set_rx_buffer(io, phys_rx_ring);
  rtl8139_rx_buffer = virt_rx_ring;
  rtl8139_cur_rx = 0;

  u32int tx_desc = 4;
  u32int tx_buf_size = 1546;

  for(i = 0; i < 4; i++) {
    rtl8139_tx_buffers[i].virt = kmalloc_p(tx_buf_size,
        &rtl8139_tx_buffers[i].phys);

    int j;
    u8int* buf = (u8int*)rtl8139_tx_buffers[i].virt;
    for(j = 0; j < 50; j++) {
      buf[j] = i;
    }
  }

  rtl8139_tx_desc = 0;
  rtl8139_reset_rx_stats(io);
  rtl8139_enable_tx_rx(io);

  register_interrupt_handler(irq, &rtl8139_interrupt);
  rtl8139_enable_interrupts(io);
}
