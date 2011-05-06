#ifndef PCI_H
#define PCI_H

#include "io.hpp"

struct PCIBus {
  IOPort io1;
  IOPort io2;
  IOPort var_io;

  u8 bus;

  u8 configb(u8 device, u8 var);
  u32 configl(u8 device, u8 var);

  const char* class2name(int klass);
  bool detect();
  void scan();

  void init();
};

extern PCIBus pci_bus;

#endif
