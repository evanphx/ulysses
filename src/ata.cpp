
#include "common.hpp"
#include "console.hpp"
#include "ata.hpp"
#include "pci.hpp"
#include "cpu.hpp"
#include "isr.hpp"

#include "block_buffer.hpp"

namespace ata {

#define ATA_SR_BUSY    0x80
#define ATA_SR_READY   0x40
#define ATA_SR_WERR    0x20
#define ATA_SR_SEEK    0x10
#define ATA_SR_DRQ     0x08
#define ATA_SR_CORR    0x04
#define ATA_SR_INDEX   0x02
#define ATA_SR_ERR     0x01
#define ATA_SR_ERROR   0x01


#define ATA_ER_BADBLOCK 0x80
#define ATA_ER_UNC      0x40
#define ATA_ER_MC       0x20
#define ATA_ER_BADID    0x10
#define ATA_ER_MCR      0x08
#define ATA_ER_ABORT    0x04
#define ATA_ER_TRACK0   0x02
#define ATA_ER_BADADDR  0x01


#define ATA_CMD_SOFT_RESET        0x08
#define ATA_CMD_DEVICE_RESET      0x08
#define ATA_CMD_READ_PIO          0x20
#define ATA_CMD_READ_PIO_EXT      0x24
#define ATA_CMD_READ_DMA          0xC8
#define ATA_CMD_READ_DMA_EXT      0x25
#define ATA_CMD_WRITE_PIO         0x30
#define ATA_CMD_WRITE_PIO_EXT     0x34
#define ATA_CMD_WRITE_DMA         0xCA
#define ATA_CMD_WRITE_DMA_EXT     0x35
#define ATA_CMD_CACHE_FLUSH       0xE7
#define ATA_CMD_CACHE_FLUSH_EXT   0xEA
#define ATA_CMD_PACKET            0xA0
#define ATA_CMD_IDENTIFY_PACKET   0xA1
#define ATA_CMD_IDENTIFY          0xEC

#define ATA_CMD_SETMULTI          0xC6


#define ATA_CTRL_DISABLE_IRQ      0x2
#define ATA_CTRL_ENABLE_IRQ       0x0


#define      ATAPI_CMD_READ       0xA8
#define      ATAPI_CMD_EJECT      0x1B

#define ATA_IDENT_DEVICETYPE   0
#define ATA_IDENT_CYLINDERS    2
#define ATA_IDENT_HEADS        6
#define ATA_IDENT_SECTORS      12
#define ATA_IDENT_SERIAL       20
#define ATA_IDENT_MODEL        54
#define ATA_IDENT_CAPABILITIES 98
#define ATA_IDENT_FIELDVALID   106
#define ATA_IDENT_MAX_LBA      120
#define ATA_IDENT_COMMANDSETS  164
#define ATA_IDENT_MAX_LBA_EXT  200


#define IDE_ATA        0x00
#define IDE_ATAPI      0x01

#define ATA_MASTER     0x00
#define ATA_SLAVE      0x01


#define ATA_REG_DATA       0x00
#define ATA_REG_ERROR      0x01
#define ATA_REG_FEATURES   0x01
#define ATA_REG_SECCOUNT0  0x02
#define ATA_REG_LBA0       0x03
#define ATA_REG_LBA1       0x04
#define ATA_REG_LBA2       0x05
#define ATA_REG_HDDEVSEL   0x06
#define ATA_REG_COMMAND    0x07
#define ATA_REG_STATUS     0x07
#define ATA_REG_SECCOUNT1  0x08
#define ATA_REG_LBA3       0x09
#define ATA_REG_LBA4       0x0A
#define ATA_REG_LBA5       0x0B
#define ATA_REG_CONTROL    0x0C
#define ATA_REG_ALTSTATUS  0x0C
#define ATA_REG_DEVADDRESS 0x0D

  // Channels:
#define      ATA_PRIMARY      0x00
#define      ATA_SECONDARY    0x01

  // Directions:
#define      ATA_READ      0x00
#define      ATA_WRITE     0x01


#define ATA_CAPA_LBA      0x2
#define ATA_CS_LBA48      (1 << 26)

  void fixstring(u8* s, int count) {
    // Make it a power of 2
    if(count & 0x1) count--;

    u8* p = s;
    u8* end = s + count;

    /* convert from big-endian to host byte order */
    for(p = end ; p != s;) {
      u16 *pp = (u16*)(p -= 2);
      u16 val = *pp;

      *pp = (val & 0xff) << 8 | (val >> 8);
    }

    /* strip leading blanks */
    while(s != end && *s == ' ') s++;

    /* compress internal blanks and strip trailing blanks */
    while(s != end && *s) {
      if(*s++ != ' ' || (s != end && *s && *s != ' ')) {
        *p++ = *(s-1);
      }
    }

    /* wipe out trailing garbage */
    while(p != end) *p++ = '\0';
  }

  void Disk::show_status() {
    u8 s = status();
    console.printf("ata: status: (%d) ", (int)s);

    if(s & ATA_SR_ERROR) {
      console.printf("error (");
      u8 which = io_.inb(ATA_REG_ERROR);

      if(which & ATA_ER_BADADDR) {
        console.printf("bad_address ");
      }

      if(which & ATA_ER_TRACK0) {
        console.printf("track0 ");
      }

      if(which & ATA_ER_ABORT) {
        console.printf("abort ");
      }

      if(which & ATA_ER_MCR) {
        console.printf("media_change_request ");
      }

      if(which & ATA_ER_BADID) {
        console.printf("bad_id ");
      }

      if(which & ATA_ER_MC) {
        console.printf("media_changed ");
      }

      if(which & ATA_ER_UNC) {
        console.printf("ecc ");
      }

      if(which & ATA_ER_BADBLOCK) {
        console.printf("bad_block/crc ");
      }

      console.printf(") ");
    }

    if(s & ATA_SR_INDEX) {
      console.printf("index ");
    }

    if(s & ATA_SR_CORR) {
      console.printf("ecc ");
    }

    if(s & ATA_SR_DRQ) {
      console.printf("drq ");
    }

    if(s & ATA_SR_SEEK) {
      console.printf("seek ");
    }

    if(s & ATA_SR_WERR) {
      console.printf("werr ");
    }

    if(s & ATA_SR_READY) {
      console.printf("ready ");
    }

    if(s & ATA_SR_BUSY) {
      console.printf("busy ");
    }

    console.printf("\n");
  }

  // Read the status of the disk, but use the alt status port
  // (ie, read from the control ioport) so that we don't clear
  // the irq status.
  u8 Disk::status() {
    return control_.inb();
  }

  bool Disk::ready_p() {
    return (status() & ATA_SR_READY) == ATA_SR_READY;
  }

  bool Disk::error_p() {
    return (status() & ATA_SR_ERROR) == ATA_SR_ERROR;
  }

  bool Disk::busy_p() {
    return (status() & ATA_SR_BUSY) == ATA_SR_BUSY;
  }

  bool Disk::drq_p() {
    return (status() & ATA_SR_DRQ) == ATA_SR_DRQ;
  }

  bool Disk::success_p() {
    return (status() & (ATA_SR_READY | ATA_SR_BUSY | ATA_SR_ERR)) == ATA_SR_READY;
  }

  void Disk::read_pio(u8* c_buf, int count) {
    u32* buf = (u32*)c_buf;
    u32 longs = count / 4;

    for(u32 i = 0; i < longs; i++) {
      buf[i] = io_.inl(ATA_REG_DATA);
    }
  }

  bool Disk::lba48_p() {
    if(info_.capability & ATA_CAPA_LBA) {
      return (info_.command_sets & ATA_CS_LBA48) != 0;
    }

    return false;
  }

  bool Disk::lba28_p() {
    return (info_.capability & ATA_CAPA_LBA) != 0;
  }
  
  bool Disk::identify() {
    disable_irq();

    memset((u8*)&info_, 0, sizeof(DriveInfo));

    io_.outb(ATA_CMD_IDENTIFY, ATA_REG_COMMAND);

    while(busy_p());

    if(!drq_p()) {
      enable_irq();
      return false;
    }

    read_pio((u8*)&info_, sizeof(info_));

    ata::fixstring(info_.model, 40);

    enable_irq();

    if((info_.capability & ATA_CAPA_LBA) == 0) {
      console.printf("ata: LBA not support, skipping drive\n");
      return false;
    }

    return true;
  }

  void Disk::show_info() {
    console.printf("%s: %s, %dMB + %dkB cache\n",
        name_.c_str(), info_.model,
        info_.lba_capacity / 2048, info_.buf_size / 2);
  }

  bool Disk::select() {
    io_.outb(select_, ATA_REG_HDDEVSEL);
    u8 check = io_.inb(ATA_REG_HDDEVSEL);

    return check == select_;
  }

  void Disk::reset() {
    io_.outb(ATA_CMD_DEVICE_RESET, ATA_REG_COMMAND);
  }

  void Disk::clear_irq() {
    // When you read the normal status register, the device
    // clears it's irq status so it will fire another irq.
    io_.inb(ATA_REG_STATUS);
  }

  void Disk::request_lba(u32 block, u8 count) {
    ASSERT(count > 0);

    // We have to read the status (drq_p) does that to reset
    // the irq levels it seems.
    ASSERT(!drq_p());

    u32 ctl = 0x08;

    control_.outb(ctl);
    io_.outb(count, ATA_REG_SECCOUNT0);
    io_.outb((block >> 0 ) & 0xff, ATA_REG_LBA0);
    io_.outb((block >> 8)  & 0xff, ATA_REG_LBA1);
    io_.outb((block >> 16) & 0xff, ATA_REG_LBA2);
    io_.outb(((block >> 24) & 0xff) | 0x40, ATA_REG_HDDEVSEL);

    // send the read command
    io_.outb(ATA_CMD_READ_PIO, ATA_REG_COMMAND);

    ASSERT(success_p());
  }

  void Disk::disable_irq() {
    control_.outb(ATA_CTRL_DISABLE_IRQ);
  }

  void Disk::enable_irq() {
    control_.outb(ATA_CTRL_ENABLE_IRQ);
  }

  // Obviously only works in BlockSize is >= 512, but that should always
  // be true.
  const static int Block2Sector = block::BlockSize / 512;

  void Disk::fulfill(block::Buffer* buffer) {
    block::RegionRange& range = buffer->range();

    u32 sector = range.sector();
    u32 num_sectors = range.num_sectors();

    interrupt_.add_request(buffer);

    select();
    enable_irq();
    request_lba(sector, num_sectors);
  }

  void Disk::wait_til_ready() {
    while(!ready_p()) cpu::halt();
  }

  void Disk::read_block(u32 block, u8* buffer) {
    disable_irq();
    select();
    request_lba(block * Block2Sector, 1 * Block2Sector);
    wait_til_ready();
    read_pio(buffer, block::BlockSize);
    enable_irq();
  }

  void ATAInterrupt::handle(Registers* regs) {
    disk_->clear_irq();

    block::Buffer* buffer = request_buffer_;
    if(!buffer) {
      console.printf("in noop ata interrupt\n");
      return;
    }

    ASSERT(disk_->success_p());

    block::RegionRange& range = buffer->range();
    u32 num_bytes = range.num_bytes();

    if(read_bytes_ >= num_bytes) {
      console.printf("weird, should be done with this request.\n");
      return;
    }

    ASSERT(buffer->byte_size() >= num_bytes);

    // console.printf("reading %d, offset=%d\n" bytes_per_read_, read_bytes_);

    disk_->read_pio(buffer->data() + read_bytes_, bytes_per_read_);

    read_bytes_ += bytes_per_read_;

    if(read_bytes_ >= num_bytes) {
      ASSERT(!disk_->drq_p());

      buffer->set_full();

      request_buffer_ = 0;
    }
  }

  void ATAInterrupt::add_request(block::Buffer* buffer) {
    request_buffer_ = buffer;
    read_bytes_ = 0;
    bytes_per_read_ = 512;
  }

  const static u16 default_ports[] = {0x1f0, 0x170, 0x1e8, 0x168, 0x1e0, 0x160 };
  const static u16 default_irqs[] =  {14, 15, 11, 10, 8, 12 };
  const static int defaults = 6;
  const static int unit_per_port = 2;

  Disk* probe(u32 port_no, int unit, const char* name) {
    IOPort port;
    port.port = port_no;

    u8 select =  0xA0 | (unit << 4);
    port.outb(select, ATA_REG_HDDEVSEL);
    u8 check = port.inb(ATA_REG_HDDEVSEL);

    if(check != select) return 0;

    Disk* disk = new(kheap) Disk(name, port_no, unit);

    if(!disk->identify()) {
      kfree(disk);
      return 0;
    }

    return disk;
  }

  void detect() {
    pci::DeviceList::Iterator i = pci_bus.devices.begin();
    while(i.more_p()) {
      pci::Device* dev = i.advance();
      if(dev->device_id() == 0x7010) {
        u32 timing = dev->configl(0x40);
        if((timing & 0x80008000) == 0) {
          console.printf("ata: no ports enabled.\n");
          return;
        }

        int devices = 0;

        for(int i = 0; i < defaults; i++) {
          for(int u = 0; u < unit_per_port; u++) {
            const char name[4] = {'a', 'd', 'a' + devices++, 0 };
            Disk* disk = probe(default_ports[i], u, name);
            if(disk) {
              interrupt::register_interrupt(default_irqs[i],
                                            disk->interrupt_handler());
              block::registry.add(disk);
              disk->show_info();
              disk->detect_partitions();
            }
          }
        }
      }
    }
  }
}
