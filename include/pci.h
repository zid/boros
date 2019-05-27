#ifndef PCI_H
#define PCI_H

#include <types-pci.h>

void pci_init(void);
void pci_write_u32(struct pci_device p, u8 offset, u32 data);
u32  pci_read_u32(struct pci_device p, u8 offset);
u16  pci_read_u16(struct pci_device p, u8 offset);

void pcie_init(u64 base);
void pcie_write_u32(struct pcie_device p, u32 offset, u32 data);
u32  pcie_read_u32(struct pcie_device p, u32 offset);
u16  pcie_read_u16(struct pcie_device p, u32 offset);

#endif
