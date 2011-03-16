
struct pci_device;

typedef struct pci_bus {
  struct pci_device* devices;
} pci_bus;

void init_pci();
