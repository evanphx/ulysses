#include "monitor.hpp"
#include "pci.hpp"

#ifdef INCLUDE_PCI_DB
#include "pci_db.hpp"
#endif

#include "rtl8139.hpp"
#include "kheap.hpp"
#include "ata.hpp"

#define CMD(bus, device, var)   (0x80000000 | (bus << 16) | (device << 8) | (var & ~3))

#define PCI_VENDOR_ID  0x00
#define PCI_DEVICE_ID  0x02
#define PCI_CLASS_REVISION	0x08
#define PCI_HEADER_VAR 0x0e

#define PCI_HEADER_TYPE_NORMAL 0
#define PCI_PRIMARY_BUS 0x18

#define PCI_SUB_VENDOR_ID 0x2c
#define PCI_SUB_DEVICE_ID 0x2e

#define PCI_BAR_0 0x10
#define PCI_BAR_1 0x14
#define PCI_BAR_2 0x18
#define PCI_BAR_3 0x1C
#define PCI_BAR_4 0x20
#define PCI_BAR_5 0x24
#define PCI_IRQ_PIN 0x3d
#define PCI_IRQ_LINE 0x3c

pci::Bus pci_bus;

namespace pci {

  const char* Device::class2name(int klass) {
    switch(klass) {
    case 0x101:
      return "IDE Controller";
    case 0x200:
      return "Ethernet Adapter";
    case 0x300:
      return "VGA Adapter";
    case 0x600:
      return "PCI Host";
    case 0x601:
      return "ISA Bridge";
    }

    if(klass >= 0x100 && klass < 0x200) {
      return "Unknown Storage";
    } else if(klass >= 0x200 && klass < 0x300) {
      return "Unknown Network";
    } else if(klass >= 0x300 && klass < 0x400) {
      return "Unknown Display";
    } else if(klass >= 0x400 && klass < 0x500) {
      return "Unknown Multimedia";
    } else if(klass >= 0x500 && klass < 0x600) {
      return "Unknown Memory";
    } else if(klass >= 0x600 && klass < 0x700) {
      return "Unknown Bridge";
    } else if(klass >= 0x700 && klass < 0x800) {
      return "Unknown Comm";
    } else if(klass >= 0x800 && klass < 0x900) {
      return "Unknown System";
    } else if(klass >= 0x900 && klass < 0xa00) {
      return "Unknown Input";
    } else if(klass >= 0xa00 && klass < 0xb00) {
      return "Unknown Docking";
    } else if(klass >= 0xb00 && klass < 0xc00) {
      return "Unknown Processor";
    } else if(klass >= 0xc00 && klass < 0xd00) {
      return "Unknown Serial";
    }

    return "Unknown Device";
  }

  u8 Bus::configb(u8 device, u8 var) {
    io2.outl(CMD(bus, device, var));
    return var_io.inb(var & 3);
  }

  u16 Bus::configw(u8 device, u8 var) {
    io2.outl(CMD(bus, device, var));
    return var_io.inw();
  }

  u32 Bus::configl(u8 device, u8 var) {
    io2.outl(CMD(bus, device, var));
    return var_io.inl();
  }

  void Bus::write_configl(u8 device, u8 var, u32 val) {
    io2.outl(CMD(bus, device, var));
    var_io.outl(val);
  }

  void Device::show() {
    const char* vendor = "unknown";

#ifdef INCLUDE_PCI_DB
    for(u32 i = 0; i < PCI_VENTABLE_LEN; i++) {
      if(PciVenTable[i].VenId == vendor_id()) {
        vendor = PciVenTable[i].VenShort;
      }
    }
#endif

    const char* device = "unknown";

#ifdef INCLUDE_PCI_DB
    for(u32 i = 0; i < PCI_DEVTABLE_LEN; i++) {
      if(PciDevTable[i].VenId == vendor_id() &&
          PciDevTable[i].DevId == device_id()) {
        device = PciDevTable[i].Chip;
      }
    }
#endif

    console.printf("%04x:%04x:%04x   %s, %s, %s\n", 
                   vendor_id(), device_id(), klass_,
                   vendor, device, class2name(klass_));
  }

#define PCI_BAR_IO_TYPE 0x1
#define PCI_BAR_IO_MASK (~0x3)
#define PCI_BAR_MEM_MASK (~0xf)

  void Device::read_bar(int which, u32 base) {
    u32 bar = bus_->configl(device_, base);

    // find out the size
    bus_->write_configl(device_, base, ~0);
    u32 sz = bus_->configl(device_, base);

    bus_->write_configl(device_, base, bar);

    // If the size is 0 or -1, then this isn't a valid bar.
    if(sz == 0 || sz == ~0UL) return;

    Device::ResourceType type;

    if((bar & PCI_BAR_IO_TYPE) == PCI_BAR_IO_TYPE) {
      type = Device::eIO;
      bar = bar & PCI_BAR_IO_MASK;
      sz  = sz  & PCI_BAR_IO_MASK;
      if(!sz) return;
    } else {
      type = Device::eMemory;
      bar = bar & PCI_BAR_MEM_MASK;
      sz  = sz  & PCI_BAR_MEM_MASK;
      if(!sz) return;
    }

    sz = sz & ~(sz-1);
    --sz;

    resources_[which] = Device::Resource(type, bar, sz);
  }

  void Device::read_settings() {
    u32 irq = bus_->configb(device_, PCI_IRQ_PIN);
    if(irq) {
      irq_ = bus_->configb(device_, PCI_IRQ_LINE);
    }

    read_bar(0, PCI_BAR_0);
    read_bar(1, PCI_BAR_1);
    read_bar(2, PCI_BAR_2);
    read_bar(3, PCI_BAR_3);
    read_bar(4, PCI_BAR_4);
    read_bar(5, PCI_BAR_5);

    if(bridge_) {
      bus_->write_configl(device_, PCI_PRIMARY_BUS, 1);
      // u32 bus_num = bus_->configl(device_, PCI_PRIMARY_BUS);
    }
  }

  void Bus::scan() {
    //console.write("Scanning PCI bus:\n");

    for(int device = 0; device < 0x100; device++) {
      u32 vendor = configl(device, PCI_VENDOR_ID);

      if(vendor == 0xffffffff || vendor == 0x0) continue;

      u32 header = configb(device, PCI_HEADER_VAR);

      bool bridge = (header != PCI_HEADER_TYPE_NORMAL);

      u32 klass = configl(device, PCI_CLASS_REVISION) >> 16;

      Device* dev = new(kheap) Device(this, vendor, device, klass, bridge);
      dev->read_settings();

      devices.append(dev);
    }
  }

  bool Bus::detect() {
    io1.outb(1);
    int val = io2.inb();

    io2.outl(0x80000000);
    bool valid = io2.inl() == 0x80000000;

    io2.outl(val);
    return valid;
  }

  void Bus::init() {
    io1.port = 0xCFB;
    io2.port = 0xCF8;
    var_io.port = 0xCFC;
    bus = 0;

    if(!detect()) {
      console.write("No PCI bus detected.\n");
      return;
    }

    scan();

    RTL8139::detect();
    ata::detect();
  }

}
