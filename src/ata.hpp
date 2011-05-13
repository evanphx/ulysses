#ifndef ATA_HPP
#define ATA_HPP

#include "string.hpp"

namespace ata {
  struct DriveInfo {
    u16	config;		/* lots of obsolete bit flags */
    u16	cyls;		/* "physical" cyls */
    u16	reserved2;	/* reserved (word 2) */
    u16	heads;		/* "physical" heads */
    u16	track_bytes;	/* unformatted bytes per track */
    u16	sector_bytes;	/* unformatted bytes per sector */
    u16	sectors;	/* "physical" sectors per track */
    u16	vendor0;	/* vendor unique */
    u16	vendor1;	/* vendor unique */
    u16	vendor2;	/* vendor unique */
    u8	serial_no[20];	/* 0 = not_specified */
    u16	buf_type;
    u16	buf_size;	/* 512 byte increments; 0 = not_specified */
    u16	ecc_bytes;	/* for r/w long cmds; 0 = not_specified */
    u8	fw_rev[8];	/* 0 = not_specified */
    u8	model[40];	/* 0 = not_specified */
    u8	max_multsect;	/* 0=not_implemented */
    u8	vendor3;	/* vendor unique */
    u16	dword_io;	/* 0=not_implemented; 1=implemented */
    u8	vendor4;	/* vendor unique */
    u8	capability;	/* bits 0:DMA 1:LBA 2:IORDYsw 3:IORDYsup*/
    u16	reserved50;	/* reserved (word 50) */
    u8	vendor5;	/* vendor unique */
    u8	tPIO;		/* 0=slow, 1=medium, 2=fast */
    u8	vendor6;	/* vendor unique */
    u8	tDMA;		/* 0=slow, 1=medium, 2=fast */
    u16	field_valid;	/* bits 0:cur_ok 1:eide_ok */
    u16	cur_cyls;	/* logical cylinders */
    u16	cur_heads;	/* logical heads */
    u16	cur_sectors;	/* logical sectors per track */
    u16	cur_capacity0;	/* logical total sectors on drive */
    u16	cur_capacity1;	/*  (2 words, misaligned int)     */
    u8	multsect;	/* current multiple sector count */
    u8	multsect_valid;	/* when (bit0==1) multsect is ok */
    u32	lba_capacity;	/* total number of sectors */
    u16	dma_1word;	/* single-word dma info */
    u16	dma_mword;	/* multiple-word dma info */
    u16  eide_pio_modes; /* bits 0:mode3 1:mode4 */
    u16  eide_dma_min;	/* min mword dma cycle time (ns) */
    u16  eide_dma_time;	/* recommended mword dma cycle time (ns) */
    u16  eide_pio;       /* min cycle time (ns), no IORDY  */
    u16  eide_pio_iordy; /* min cycle time (ns), with IORDY */
    u16  word69;
    u16  word70;
    /* HDIO_GET_IDENTITY currently returns only words 0 through 70 */
    u16  word71;
    u16  word72;
    u16  word73;
    u16  word74;
    u16  word75;
    u16  word76;
    u16  word77;
    u16  word78;
    u16  word79;
    u16  word80;
    u16  word81;
    u32  command_sets;	/* bits 0:Smart 1:Security 2:Removable 3:PM */
    u16  word83;		/* bits 14:Smart Enabled 13:0 zero */
    u16  word84;
    u16  word85;
    u16  word86;
    u16  word87;
    u16  dma_ultra;
    u16	word89;		/* reserved (word 89) */
    u16	word90;		/* reserved (word 90) */
    u16	word91;		/* reserved (word 91) */
    u16	word92;		/* reserved (word 92) */
    u16	word93;		/* reserved (word 93) */
    u16	word94;		/* reserved (word 94) */
    u16	word95;		/* reserved (word 95) */
    u16	word96;		/* reserved (word 96) */
    u16	word97;		/* reserved (word 97) */
    u16	word98;		/* reserved (word 98) */
    u32 lba_ext_capacity0;
    u32 lba_ext_capacity1;
    u16	word103;	/* reserved (word 103) */
    u16	word104;	/* reserved (word 104) */
    u16	word105;	/* reserved (word 105) */
    u16	word106;	/* reserved (word 106) */
    u16	word107;	/* reserved (word 107) */
    u16	word108;	/* reserved (word 108) */
    u16	word109;	/* reserved (word 109) */
    u16	word110;	/* reserved (word 110) */
    u16	word111;	/* reserved (word 111) */
    u16	word112;	/* reserved (word 112) */
    u16	word113;	/* reserved (word 113) */
    u16	word114;	/* reserved (word 114) */
    u16	word115;	/* reserved (word 115) */
    u16	word116;	/* reserved (word 116) */
    u16	word117;	/* reserved (word 117) */
    u16	word118;	/* reserved (word 118) */
    u16	word119;	/* reserved (word 119) */
    u16	word120;	/* reserved (word 120) */
    u16	word121;	/* reserved (word 121) */
    u16	word122;	/* reserved (word 122) */
    u16	word123;	/* reserved (word 123) */
    u16	word124;	/* reserved (word 124) */
    u16	word125;	/* reserved (word 125) */
    u16	word126;	/* reserved (word 126) */
    u16	word127;	/* reserved (word 127) */
    u16	security;	/* bits 0:suuport 1:enabled 2:locked 3:frozen */
    u16	reserved[127];
  };

  void fixstring(u8* s, int count);

  class Disk {
    sys::FixedString<8> name_;
    IOPort io_;
    IOPort control_;
    u8 select_;
    DriveInfo info_;

  public:
    Disk(const char* name, u16 port, u8 unit)
      : name_(name)
      , select_(0xA0 | (unit << 4))
    {
      io_.port = port;
      control_.port = port + 0x206;
    }

  public:
    void show_info();
    bool identify();
    void read_pio(u8* buf, int count);
    bool select();
    void request_lba(u32 block, u8 count);
    void show_status();

    void disable_irq();
    void enable_irq();

    bool lba48_p();
    bool lba28_p();

    bool ready_p();
    bool error_p();
    bool drq_p();
    bool busy_p();
  };

  void detect();
  Disk* probe(u32 port, int unit);
};

#endif
