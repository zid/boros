#include <print.h>
#include <types.h>
#include <device.h>
#include <mem.h>
#include <bug.h>

enum VIRTIO_PCI_CAP
{
	VIRTIO_PCI_CAP_COMMON_CFG = 1,
	VIRTIO_PCI_CAP_NOTIFY_CFG,
	VIRTIO_PCI_CAP_ISR_CFG,
	VIRTIO_PCI_CAP_DEVICE_CFG,
	VIRTIO_PCI_CAP_PCI_CFG
};

enum DEVICE_STATUS
{
	RESET       = 0,
	ACKNOWLEDGE = 1,
	DRIVER      = 2,
	DRIVER_OK   = 4,
	FEATURES_OK = 8,
	DEVICE_NEEDS_RESET = 64,
	FAILED      = 128
};

struct virtio_pci_cap
{
	u8 cap_vndr;
	u8 cap_next;
	u8 cap_len;
	u8 cfg_type;
	u8 bar;
	u8 pad[3];
	u32 offset;
	u32 length;
};

struct virtio_pci_common_cfg
{
	u32 device_feature_select;
	u32 device_feature;
	u32 driver_feature_select;
	u32 driver_feature;
	u16 msix_config;
	u16 num_queues;
	u8 device_status;
	u8 config_generation;

	u16 queue_select;
	u16 queue_size;
	u16 queue_msix_vector;
	u16 queue_enable;
	u16 queue_notify_off;
	u64 queue_desc;
	u64 queue_avail;
	u64 queue_used;
};

static struct virtio_pci_cap
find_cap(struct device *d, u8 caps, enum VIRTIO_PCI_CAP needle)
{
	struct virtio_pci_cap vpc;
	u32 data;

	do
	{
		data = device_read_u32(d, caps);
		vpc.cap_vndr = data;
		vpc.cap_next = data>>8;
		vpc.cap_len  = data>>16;
		vpc.cfg_type = data>>24;

		data = device_read_u32(d, caps+4);
		vpc.bar = data;

		data = device_read_u32(d, caps+8);
		vpc.offset = data;

		data = device_read_u32(d, caps+12);
		vpc.length = data;

		if(vpc.cap_vndr == 9 && vpc.cfg_type == needle)
			return vpc;

		caps = vpc.cap_next;
	} while(vpc.cap_next);

	/* Not found */
	vpc.length = 0;

	return vpc;
}

void virtio_init(struct device *d)
{
	struct virtio_pci_cap vpc;
	volatile struct virtio_pci_common_cfg *cfg;
	u32 base;
	volatile u8 *mmio, *isr;
	u8 caps;

	printf("[virtio_net]: Init\n");
	caps = device_read_u32(d, 0x34) & 0xFC;
	if(!caps)
	{
		printf("[virtio_net]: error: No device caps");
		return;
	}

	vpc = find_cap(d, caps, VIRTIO_PCI_CAP_DEVICE_CFG);

	if(!vpc.length)
		bug("Couldn't find MMIO BAR for virtio");

	/* Read the MMIO BAR */
	base = device_read_u32(d, 0x10 + vpc.bar*4);
	base &= ~0xF;
	printf("[virtio_net]: MMIO base: %x\n", base + vpc.offset);

	mmio = (u8 *)phys_to_virt(base + vpc.offset);
	mmap((u64)mmio, base + vpc.offset, 0x1000, PT_WR | PT_PRESENT);

	vpc = find_cap(d, caps, VIRTIO_PCI_CAP_COMMON_CFG);
	if(!vpc.length)
		bug("Couldn't find config BAR for virtio");

	base = device_read_u32(d, 0x10 + vpc.bar*4);
	base &= ~0xF;
	printf("[virtio_net]: Config base: %x\n", base + vpc.offset);

	cfg = (void *)phys_to_virt(base + vpc.offset);
	mmap((u64)cfg, base + vpc.offset, 0x1000, PT_WR | PT_PRESENT);

	cfg->device_status = RESET;
	while(cfg->device_status)
		;

	cfg->device_status = ACKNOWLEDGE;
	cfg->device_status |= DRIVER;
	cfg->driver_feature = cfg->device_feature;
	cfg->device_status = FEATURES_OK;
	if(!(cfg->device_status & FEATURES_OK))
	{
		cfg->device_status = FAILED;
		return;
	}

	printf("[virtio_net]: Features accepted\n");

	vpc = find_cap(d, caps, VIRTIO_PCI_CAP_ISR_CFG);
	if(!vpc.length)
		bug("Couldn't find ISR BAR for virtio");

	base = device_read_u32(d, 0x10 + vpc.bar*4);
	base &= ~0xF;
	printf("[virtio_net]: ISR base: %x\n", base + vpc.offset);

	isr = (void *)phys_to_virt(base + vpc.offset);
	mmap((u64)isr, base + vpc.offset, 0x1000, PT_WR | PT_PRESENT);

	printf("[virtio_net]: MAC: %02x:%02x:%02x:%02x:%02x:%02x\n",
		mmio[0x0],
		mmio[0x1],
		mmio[0x2],
		mmio[0x3],
		mmio[0x4],
		mmio[0x5]
	);

}

