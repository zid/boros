#include <device.h>
#include <pci.h>
#include <bug.h>

void device_write_u32(struct device *d, u32 offset, u32 data)
{
	switch(d->bus)
	{
		case BUS_ISA:
			bug("Unsupported ISA write");
		break;
		case BUS_PCI:
			pci_write_u32(d->pci, offset, data);
		break;
		case BUS_PCIE:
			pcie_write_u32(d->pcie, offset, data);
		break;
		default:
			bug("Write to unknown bus type");
		break;
	}
}

u32 device_read_u32(struct device *d, u32 offset)
{
	switch(d->bus)
	{
		case BUS_ISA:
			bug("Unsupported ISA read");
		break;
		case BUS_PCI:
			if(offset >= 253)
				bug("PCI read out of range");
			return pci_read_u32(d->pci, offset);
		break;
		case BUS_PCIE:
			return pcie_read_u32(d->pcie, offset);
		break;
		default:
			bug("Read from unknown bus type");
		break;
	}
}

u16 device_read_u16(struct device *d, u32 offset)
{
	switch(d->bus)
	{
		case BUS_ISA:
			bug("Unsupported ISA read");
		break;
		case BUS_PCI:
			if(offset >= 255)
				bug("PCI read out of range");
			return pci_read_u16(d->pci, offset);
		case BUS_PCIE:
			return pcie_read_u16(d->pcie, offset);
		break;
		default:
			bug("Read from unknown bus type");
		break;
	}
}
