#ifndef DEVICE_H
#define DEVICE_H

#include <types-pci.h>

struct isa_device
{
	/* Move me to isa.c when you implement */
};

enum BUS_TYPE {
	BUS_ISA,
	BUS_PCI,
	BUS_PCIE
};

struct device {
	enum BUS_TYPE bus;

	union {
		struct isa_device isa;
		struct pci_device pci;
		struct pcie_device pcie;
	};
};

void device_write_u32(struct device *d, u32 offset, u32 data);
u32  device_read_u32(struct device *d, u32 offset);
u16  device_read_u16(struct device *d, u32 offset);

#endif
