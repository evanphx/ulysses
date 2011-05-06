#include "monitor.h"
#include "isr.h"
#include "eth.h"
#include "kheap.h"
#include "rtl8139.h"

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

RTL8139 rtl8139;

static struct netif rtl8139_lwip_netif;
static struct ip_addr rtl8139_ipaddr = {0};
static struct ip_addr rtl8139_netmask = {0};
static struct ip_addr rtl8139_gw = {0};

// END STATE

char* eth_mac() {
  return rtl8139.mac;
}

void RTL8139::read_mac() {
  int i;
  for(i = 0; i < 6; i++) {
    mac[i] = ctrl.inb(MAC0 + i);
  }
}

void RTL8139::power_on() {
  ctrl.outb(0, Config1);
}

void RTL8139::pm_wakeup() {
  u8 config = ctrl.inb(Config1);
  config |= Cfg1_PM_Enable;
  ctrl.outb(config, Config1);
}

void RTL8139::reset() {
  ctrl.outb(CmdReset, ChipCmd);

  console.write("rtl8139: Resetting...");
  for(;;) {
    u8int byte = ctrl.inb(ChipCmd);
    if((byte & CmdReset) == 0) {
      console.write("reset\n");
      return;
    }
  }

  console.write("rtl8139: Timeout resetting.\n");
}

void RTL8139::enable_tx_rx() {
  ctrl.outb(CmdTxEnb | CmdRxEnb, ChipCmd);
}

void RTL8139::set_config() {
  ctrl.outl(default_rx_config, RxConfig);
  ctrl.outl(default_tx_config, TxConfig);
}

void RTL8139::unlock() {
  ctrl.outb(Cfg9346_Unlock, Cfg9346);
}

void RTL8139::lock() {
  ctrl.outb(Cfg9346_Lock, Cfg9346);
}

void RTL8139::set_clock(int run) {
  ctrl.outb(run ? 'R' : 'H', HltClk);
}

void RTL8139::set_rx_buffer(u32int addr) {
  ctrl.outl(addr, RxBuf);
}

void RTL8139::reset_rx_stats() {
  ctrl.outl(0, RxMissed);
}

void RTL8139::enable_interrupts() {
  ctrl.outw(AllInterrupts, IntrMask);
}

u16 RTL8139::ack() {
  u16 s = status.inw();
  status.outw(s);
  return s;
}

u32 RTL8139::last_tx_status() {
  int entry = tx_desc;
  if(entry == 0) {
    entry = 3;
  } else {
    entry--;
  }

  return ctrl.inl(TxStatus0 + entry*4);
}

void RTL8139::receive(u16int status) {
  while((ctrl.inb(ChipCmd) & RxBufEmpty) == 0) {
    int offset = cur_rx % default_rx_buf_len;

    u16int buf_addr = ctrl.inw(RxBufAddr);
    u16int buf_ptr =  ctrl.inw(RxBufPtr);
    u8int cmd = ctrl.inb(ChipCmd);

    u32int* buf_start = (u32int*)(rx_buffer + offset);
    u8int*  u8buf_start = (u8int*)(rx_buffer + offset);
    u32int rx_status = *buf_start;
    int rx_size = rx_status >> 16;

    console.write("rtl8139: receive: buf_addr=");
    console.write_hex(buf_addr);
    console.write(", buf_ptr=");
    console.write_hex(buf_ptr);
    console.write(", cmd=");
    console.write_hex(cmd);
    console.write(", rx_status=");
    console.write_hex(rx_status);
    console.write(", rx_size=");
    console.write_dec(rx_size);
    console.write("\n");

    console.write("rtl8139: ring buffer=");
    console.write_hex((u32int)buf_start);
    console.write(" size=");
    console.write_dec(rx_size);
    console.write(" cur_rx=");
    console.write_dec(cur_rx);
    console.write(" offset=");
    console.write_dec(offset);
    kputs("\n");

    if(rx_status & RxErrors) {
      console.write("rtl8139: Detected frame errors.\n");
    } else {
      /*
      console.write("Frame:\n");
      for(i = 4; i < rx_size; i++) {
        console.write(" ");
        console.write_hex_byte(u8buf_start[i]);
      }

      console.write("\n");
      */

      struct pbuf* pkt = pbuf_alloc(PBUF_RAW, rx_size, PBUF_RAM);
      pbuf_take(pkt, u8buf_start + 4, rx_size);
      eth_handoff(u8buf_start + 4, rx_size);
      ethernet_input(pkt, &rtl8139_lwip_netif);
    }

    cur_rx = (cur_rx + rx_size + 4 + 3) & ~3;
    ctrl.outw(cur_rx - 16, RxBufPtr);
  }
}

static void rtl8139_interrupt(registers_t* regs) {
  u16int status = rtl8139.ack();

  if((status & AllInterrupts) == 0) return;
  if(status & RxInterrupts) rtl8139.receive(status);
  if(status & TxInterrupts) {
    int tx_status = rtl8139.last_tx_status();
    if(tx_status == TxOK) {
      kputs("Got a TxOK interrupt.\n");
    } else {
      kputs("Got a TxErr interrupt.\n");
    }
  }
}

void RTL8139::transmit(u8int* buf, int size) {
  int entry = tx_desc;

  kputs("rtl8139: Writing to tx descriptor: ");
  console.write_dec(entry);
  kputs("\n");

  console.printf("copy %p (%d) to %p\n", buf, size, tx_buffers[entry].virt);
  memcpy((u8int*)tx_buffers[entry].virt, buf, size);
  ctrl.outl(tx_buffers[entry].phys, TxAddr0 + entry*4);
  ctrl.outl(size & 0x1fff, TxStatus0 + entry*4);

  if(++tx_desc > 4) tx_desc = 0;
}

void xmit_packet(u8int* buf, int size) {
  rtl8139.transmit(buf, size);
}

err_t rtl8139_open(struct netif* netif);
err_t rtl8139_input(struct pbuf* p, struct netif* netif) {
  kputs("Would send packet out rtl8139 for lwip.\n");
  return 0;
}

err_t rtl8139_link_output(struct netif* netif, struct pbuf* p) {
  xmit_packet((u8int*)p->payload, p->len);
  return 0;
}

err_t rtl8139_output(struct netif* nif, struct pbuf* p, struct ip_addr* dest) {
  etharp_output(nif, p, dest); 
  return 0;
}

void init_rtl8139(u32int io, u8int irq) {
  rtl8139.ctrl.port = io;
  rtl8139.status.port = io + IntrStatus;

  console.printf("Detected RTL8139 at port 0x%x (irq %d)\n", io, irq);

  rtl8139.irq = irq;

  memset((u8int*)&rtl8139_lwip_netif, 0, sizeof(struct netif));

  rtl8139_ipaddr.addr = inet_addr("10.0.2.15");

  netif_add(&rtl8139_lwip_netif, &rtl8139_ipaddr, &rtl8139_netmask,
            &rtl8139_gw, 0, rtl8139_open, rtl8139_input);

  rtl8139_lwip_netif.linkoutput = rtl8139_link_output;
  rtl8139_lwip_netif.output = rtl8139_output;

  netif_set_default(&rtl8139_lwip_netif);
  netif_set_up(&rtl8139_lwip_netif);
}

void RTL8139::init() {
  power_on();

  read_mac();

  console.write("Realtek 8139: MAC ");
  for(int i = 0; i < 6; i++) {
    console.write_hex_np(mac[i]);
    if(i != 5) console.write(":");
  }

  console.write("\n");

  unlock();
  pm_wakeup();
  set_clock(0);

  reset();
  enable_tx_rx();
  set_config();

  u32int phys_rx_ring = 0;
  u32int virt_rx_ring = kmalloc_ap(default_rx_buf_len + 16, &phys_rx_ring);

  console.write("rtl8139: phys=");
  console.write_hex(phys_rx_ring);
  console.write(" virt=");
  console.write_hex(virt_rx_ring);
  console.write("\n");

  set_rx_buffer(phys_rx_ring);
  rx_buffer = virt_rx_ring;
  cur_rx = 0;

  u32int tx_desc = 4;
  u32int tx_buf_size = 1546;

  for(int i = 0; i < 4; i++) {
    tx_buffers[i].virt = kmalloc_p(tx_buf_size,
        &tx_buffers[i].phys);

    int j;
    u8int* buf = (u8int*)tx_buffers[i].virt;
    for(j = 0; j < 50; j++) {
      buf[j] = i;
    }
  }

  tx_desc = 0;
  reset_rx_stats();
  enable_tx_rx();

  register_interrupt_handler(irq, &rtl8139_interrupt);
}

err_t rtl8139_open(struct netif* netif) {
  rtl8139.init();

  memcpy((u8int*)rtl8139_lwip_netif.hwaddr, (u8int*)rtl8139.mac, 6);
  rtl8139_lwip_netif.hwaddr_len = 6;

  rtl8139.enable_interrupts();
  return 0;
}
