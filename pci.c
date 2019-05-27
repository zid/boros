#include <mem.h>
#include <print.h>
#include <pci.h>
#include <device.h>

#define PCI_CONFIG_ADDRESS 0xCF8
#define PCI_DATA_ADDRESS 0xCFC
#define PCI_ENABLE 0x80000000UL

void pci_write_u32(struct pci_device d, u8 offset, u32 data)
{
	u32 addr;

	addr = PCI_ENABLE;
	addr |= d.bus<<16 | d.dev<<11 | d.func<<8 | (offset&0xFC);

	out32(PCI_CONFIG_ADDRESS, addr);
	out32(PCI_DATA_ADDRESS, data);
}

u16 pci_read_u16(struct pci_device d, u8 offset)
{
	u32 addr;
	u32 data;

	addr = PCI_ENABLE;
	addr |= d.bus<<16 | d.dev<<11 | d.func<<8 | (offset & 0xFC);
	out32(PCI_CONFIG_ADDRESS, addr);

	data = in32(PCI_DATA_ADDRESS);

	if(offset & 2)
		data >>= 16;

	return (u16)data;
}

u32 pci_read_u32(struct pci_device d, u8 offset)
{
	u32 addr;

	addr = PCI_ENABLE;
	addr |= d.bus<<16 | d.dev<<11 | d.func<<8 | (offset & 0xFC);

	out32(PCI_CONFIG_ADDRESS, addr);

	return in32(PCI_DATA_ADDRESS);
}

static void pci_dump(struct device d)
{
	u16 device_id, vendor_id;

	vendor_id = pci_read_u16(d.pci, 0);
	device_id = pci_read_u16(d.pci, 2);

	printf("Bus: %d, Device: %d\n  [%04X:%04X]\n",
		d.pci.bus, d.pci.dev, vendor_id, device_id);
	/* TODO: Make some driver registration system
	 * and match these tuples into the table and call
	 * the registered callback instead.
	 */
	if(vendor_id == 0x8086 && (device_id == 0x100E || device_id == 0x100F))
		e1000_init(&d);

	if(vendor_id == 0x1af4 && device_id == 0x1000)
		virtio_init(&d);
}

struct pcie_config
{
	u16 vendor_id, device_id;

	u16 command, status;

	u8 revision_id, prof_if, subclass, class;
	u8 cache_line_size;
	u8 latency_timer;
	u8 header_type;
	u8 bist;

	u64 bar0, bar1, bar2;
	u64 bar3, bar4, bar5;
};

void pcie_write_u32(struct pcie_device d, u32 offset, u32 data)
{
	*(u32 *)(d.base + offset) = data;
}

u32 pcie_read_u32(struct pcie_device d, u32 offset)
{
	return *(u32 *)((u8 *)d.base + offset);
}

u16 pcie_read_u16(struct pcie_device d, u32 offset)
{
	return *(u16 *)((u8 *)d.base + offset);
}

static void pcie_handle_device(struct pcie_config *cfg)
{
	struct device d;

	d.bus = BUS_PCIE;
	d.pcie.base = cfg;

	printf("Device %04X:%04X\n", cfg->vendor_id, cfg->device_id);

	/* hack */
	if(cfg->vendor_id == 0x1AF4 && cfg->device_id == 0x1000)
		virtio_init(&d);
	if(cfg->vendor_id == 0x8086 && cfg->device_id == 0x100E)
		e1000_init(&d);
}

static void pcie_parse_device(u64 addr)
{
	struct pcie_config *cfg;

	/* Map device so we can talk to it */
	mmap(phys_to_virt(addr), addr, 0x8000, PT_WR);
	cfg = (struct pcie_config *)phys_to_virt(addr);

	/* No device present */
	if(cfg->device_id == 0xFFFF)
		return;

	pcie_handle_device(cfg);

	/* Multifunction device */
	if(cfg->header_type & 0x80)
	{
		int n;
		for(n = 1; n <= 7; n++)
		{
			struct pcie_config *t;

			t = (struct pcie_config *)phys_to_virt(addr + n*0x1000);
			if(t->device_id != 0xFFFF)
				pcie_handle_device(t);
		}
	}
}

void pcie_init(u64 base)
{
	u32 bus, device;
	printf("PCI-E Init: %lx\n", base);

	for(bus = 0; bus <= 0; bus++)
	{
		for(device = 0; device <= 31; device++)
		{
			pcie_parse_device(base | bus<<20 | device<<15);
		}
	}
}


void pci_init(void)
{
	u32 t;
	unsigned int bus, dev;

	printf("Probing PCI devices.\n");

	for(bus = 0; bus <= 255; bus++)
	{
		for(dev = 0; dev < 31; dev++)
		{
			struct device d;

			d.bus = BUS_PCI;

			d.pci.bus = bus;
			d.pci.dev = dev;
			d.pci.func = 0;

			t = pci_read_u16(d.pci, 0);
			if(t == 0xFFFF)
				continue;
			pci_dump(d);
		}
	}
}

