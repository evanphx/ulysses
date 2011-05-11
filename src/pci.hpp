#ifndef PCI_H
#define PCI_H

#include "io.hpp"
#include "list.hpp"

namespace pci {
  class Device;
  typedef sys::ExternalList<Device*> DeviceList;


  struct Bus {
    IOPort io1;
    IOPort io2;
    IOPort var_io;

    u8 bus;

    DeviceList devices;

    u8 configb(u8 device, u8 var);
    u32 configl(u8 device, u8 var);
    void write_configl(u8 device, u8 var, u32 val);

    bool detect();
    void scan();

    void init();
  };

  class Device {
  public:
    enum ResourceType {
      eNone=0, eIO, eMemory
    };

    struct Resource {
      ResourceType type;
      int base;
      int size;

      Resource(ResourceType t, int b, int s)
        : type(t)
        , base(b)
        , size(s)
      {}

      Resource()
        : type(eNone)
        , base(0)
        , size(0)
      {}
    };

  private:
    Bus* bus_;
    u32 vendor_;
    u32 device_;
    u32 klass_;
    u32 irq_;

    Resource resources_[6];

  public:
    Device(Bus* bus, u32 v, u32 d, u32 k)
      : bus_(bus)
      , vendor_(v)
      , device_(d)
      , klass_(k)
      , irq_(0)
    {}

    u32 vendor_id() {
      return vendor_ & 0xffff;
    }

    u32 device_id() {
      return vendor_ >> 16;
    }

    u32 irq() {
      return irq_;
    }

    u32 io_port(int which) {
      if(resources_[which].type == eIO) {
        return resources_[which].base;
      }

      return 0;
    }

    static const char* class2name(int klass);

    void read_settings();
    void read_bar(int which, u32 base);
    void show();
  };
}

extern pci::Bus pci_bus;

#endif
