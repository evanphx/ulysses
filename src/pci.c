#include "monitor.h"

#include "pci_db.h"
#include "rtl8139.h"

#define CMD(bus, device, var)   (0x80000000 | (bus << 16) | (device << 8) | (var & ~3))

#define PCI_VENDOR_ID  0x00
#define PCI_DEVICE_ID  0x02
#define PCI_CLASS_REVISION	0x08
#define PCI_HEADER_VAR 0x0e

#define PCI_BAR_0 0x10
#define PCI_IRQ 0x3c

static const char* class2name(int klass) {
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

static u32int pci_configb(u8int bus, u8int device, u8int var) {
  outl(0xCF8, CMD(bus, device, var));
  return inb(0xCFC + (var & 3));
}

static u32int pci_configl(u8int bus, u8int device, u8int var) {
  outl(0xCF8, CMD(bus, device, var));
  return inl(0xCFC);
}

static void pci_scan_bus() {
  int device;

  monitor_write("Scanning PCI bus:\n");

  for(device = 0; device < 0xff; device++) {
    u32int header = pci_configb(0, device, PCI_HEADER_VAR);

    u32int vendor = pci_configl(0, device, PCI_VENDOR_ID);

    if(vendor == 0xffffffff || vendor == 0x0) continue;

    u32int vendor_id = vendor & 0xffff;
    u32int device_id = vendor >> 16;

    u32int klass = pci_configl(0, device, PCI_CLASS_REVISION) >> 16;

    /* monitor_write("PCI Device: header="); */
    /* monitor_write_hex(header); */
    monitor_write_hex(vendor_id);
    monitor_write(":");
    monitor_write_hex(device_id);
    monitor_write(":");
    monitor_write_hex(klass);

    monitor_write("  ");

    int found_vendor = 0;

    int i;
    for(i = 0; i < PCI_VENTABLE_LEN; i++) {
      if(PciVenTable[i].VenId == vendor_id) {
        monitor_write(PciVenTable[i].VenShort);
        found_vendor = 1;
      }
    }

    if(!found_vendor) {
      monitor_write("Unknown vendor");
    }

    monitor_write(", ");

    int found_device = 1;

    for(i = 0; i < PCI_DEVTABLE_LEN; i++) {
      if(PciDevTable[i].VenId == vendor_id &&
          PciDevTable[i].DevId == device_id) {
        monitor_write(PciDevTable[i].Chip);
        found_device = 1;
      }
    }

    if(!found_device) {
      monitor_write("Unknown device");
    }

    monitor_write(", ");
    monitor_write(class2name(klass));

    u8int irq = pci_configb(0, device, PCI_IRQ);
    u32int io_port = pci_configl(0, device, PCI_BAR_0) & ~3;

    monitor_write(" irq=");
    monitor_write_dec(irq);
    monitor_write(", io=");
    monitor_write_hex(io_port);

    monitor_write("\n");

    if(device_id == 0x8139) {
      init_rtl8139(io_port, irq);
    }
  }
}

void init_pci() {

  outb(0xCFB, 0x1);
  int val = inl(0xCF8);
  outl(0xCF8, 0x80000000);

  if (inl (0xCF8) == 0x80000000) {
    monitor_write("Detected PCI access.\n");
  }

  outl(0xCF8, val);

  pci_scan_bus();
}
