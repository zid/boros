#include "pci.h"
#include "cpu.h"
#include "print.h"

#define PCI_CONFIG_ADDRESS 0xCF8
#define PCI_DATA_ADDRESS 0xCFC


static u16 pci_read_u16(u8 bus, u8 slot, u8 func, u8 offset)
{
	u32 addr;
	u32 data;

	addr = 0x80000000UL | bus <<16 | slot<<11 | func<<8 | (offset & 0xFC);
	out32(PCI_CONFIG_ADDRESS, addr);

	data = in32(PCI_DATA_ADDRESS);

	if(offset & 2)
		data >>= 16;

	return (u16)data;
}

static u32 pci_read_u32(u8 bus, u8 slot, u8 func, u8 offset)
{
	u32 addr;

	addr = 0x80000000UL | bus <<16 | slot<<11 | func<<8 | (offset & 0xFC);
	out32(PCI_CONFIG_ADDRESS, addr);

	return in32(PCI_DATA_ADDRESS);
}

static void pci_dump(u8 bus, u8 dev)
{
	u16 device_id, vendor_id;

	device_id = pci_read_u16(bus, dev, 0, 0);
	vendor_id = pci_read_u16(bus, dev, 0, 2);

	printf("Bus: %d, Device: %d\n  [%04X:%04X]\n", bus, dev, device_id, vendor_id);
}

void pci_init(void)
{
	u32 t;
	unsigned int bus, dev;

	for(bus = 0; bus <= 255; bus++)
	{
		for(dev = 0; dev < 31; dev++)
		{
			t = pci_read_u16(bus, dev, 0, 0);
			if(t == 0xFFFF)
				continue;
			pci_dump(bus, dev);
		}
	}
}
