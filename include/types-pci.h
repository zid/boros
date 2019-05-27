#ifndef TYPESPCI_H
#define TYPESPCI_H

#include <types.h>

struct pci_device
{
	u8 bus, dev, func;
};

struct pcie_device
{
	void *base;
};

#endif
