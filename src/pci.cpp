#include "monitor.hpp"
#include "pci.hpp"
#include "pci_db.hpp"
#include "rtl8139.hpp"

#define CMD(bus, device, var)   (0x80000000 | (bus << 16) | (device << 8) | (var & ~3))

#define PCI_VENDOR_ID  0x00
#define PCI_DEVICE_ID  0x02
#define PCI_CLASS_REVISION	0x08
#define PCI_HEADER_VAR 0x0e

#define PCI_BAR_0 0x10
#define PCI_IRQ 0x3c

PCIBus pci_bus = {{0xCFB}, {0xCF8}, {0xCFC}, 0};

const char* PCIBus::class2name(int klass) {
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

u8 PCIBus::configb(u8 device, u8 var) {
  io2.outl(CMD(bus, device, var));
  return var_io.inb(var & 3);
}

u32 PCIBus::configl(u8 device, u8 var) {
  io2.outl(CMD(bus, device, var));
  return var_io.inl();
}

void PCIBus::scan() {
  //console.write("Scanning PCI bus:\n");

  for(int device = 0; device < 0xff; device++) {
    // u32 header = configb(device, PCI_HEADER_VAR);

    u32 vendor = configl(device, PCI_VENDOR_ID);

    if(vendor == 0xffffffff || vendor == 0x0) continue;

    u32 vendor_id = vendor & 0xffff;
    u32 device_id = vendor >> 16;

    u32 klass = configl(device, PCI_CLASS_REVISION) >> 16;

    /* console.write("PCI Device: header="); */
    /* console.write_hex(header); */
    console.printf("%04x:%04x:%04x   ", vendor_id, device_id, klass);

    int found_vendor = 0;

    for(u32 i = 0; i < PCI_VENTABLE_LEN; i++) {
      if(PciVenTable[i].VenId == vendor_id) {
        console.write(PciVenTable[i].VenShort);
        found_vendor = 1;
      }
    }

    if(!found_vendor) {
      console.write("Unknown vendor");
    }

    console.write(", ");

    int found_device = 1;

    for(u32 i = 0; i < PCI_DEVTABLE_LEN; i++) {
      if(PciDevTable[i].VenId == vendor_id &&
          PciDevTable[i].DevId == device_id) {
        console.write(PciDevTable[i].Chip);
        found_device = 1;
      }
    }

    if(!found_device) {
      console.write("Unknown device");
    }

    console.write(", ");
    console.write(class2name(klass));

    u8 irq = configb(device, PCI_IRQ);
    u32 io_port = configl(device, PCI_BAR_0) & ~3;

    console.write(" irq=");
    console.write_dec(irq);
    console.write(", io=");
    console.write_hex(io_port);

    console.write("\n");

    if(device_id == 0x8139) {
      init_rtl8139(io_port, irq);
    }
  }
}

bool PCIBus::detect() {
  io1.outb(1);
  int val = io2.inb();

  io2.outl(0x80000000);
  bool valid = io2.inl() == 0x80000000;

  io2.outl(val);
  return valid;
}

void PCIBus::init() {
  if(!pci_bus.detect()) {
    console.write("No PCI bus detected.\n");
    return;
  }

  pci_bus.scan();
}
