#ifndef PCI_H
#define PCI_H
#include "types.h"
void pci_init(void);
u16 pci_read_u16(u8, u8, u8, u8);
u32 pci_read_u32(u8, u8, u8, u8);
#endif
