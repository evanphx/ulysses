#include "monitor.h"

#include "pci_db.h"

#define CMD(bus, device, var)   (0x80000000 | (bus << 16) | (device << 8) | (var & ~3))

#define PCI_VENDOR_ID  0x00
#define PCI_DEVICE_ID  0x02
#define PCI_CLASS_REVISION	0x08
#define PCI_HEADER_VAR 0x0e

static const char* class2name(int class) {
  switch(class) {
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

  if(class >= 0x100 && class < 0x200) {
    return "Unknown Storage";
  } else if(class >= 0x200 && class < 0x300) {
    return "Unknown Network";
  } else if(class >= 0x300 && class < 0x400) {
    return "Unknown Display";
  } else if(class >= 0x400 && class < 0x500) {
    return "Unknown Multimedia";
  } else if(class >= 0x500 && class < 0x600) {
    return "Unknown Memory";
  } else if(class >= 0x600 && class < 0x700) {
    return "Unknown Bridge";
  } else if(class >= 0x700 && class < 0x800) {
    return "Unknown Comm";
  } else if(class >= 0x800 && class < 0x900) {
    return "Unknown System";
  } else if(class >= 0x900 && class < 0xa00) {
    return "Unknown Input";
  } else if(class >= 0xa00 && class < 0xb00) {
    return "Unknown Docking";
  } else if(class >= 0xb00 && class < 0xc00) {
    return "Unknown Processor";
  } else if(class >= 0xc00 && class < 0xd00) {
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

    u32int class = pci_configl(0, device, PCI_CLASS_REVISION) >> 16;

    /* monitor_write("PCI Device: header="); */
    /* monitor_write_hex(header); */
    monitor_write_hex(vendor_id);
    monitor_write(":");
    monitor_write_hex(device_id);
    monitor_write(":");
    monitor_write_hex(class);

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
    monitor_write(class2name(class));

    monitor_write("\n");
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
