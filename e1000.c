#include <types.h>
#include <device.h>
#include <mem.h>
#include <int.h>
#include <print.h>
#include <net.h>
#include <cpu.h>

uintptr_t net_get_buf(void);

#define RX_NUM 16
#define TX_NUM 16

struct rx_desc {
	volatile u64 addr;
	volatile u16 len;
	volatile u16 checksum;
	volatile u8 status;
	volatile u8 errors;
	volatile u16 special;
} __attribute__((packed));

struct tx_desc {
	volatile u64 addr;
	volatile u16 len;
	volatile u8 cso;
	volatile u8 cmd;
	volatile u8 status;
	volatile u8 css;
	volatile u16 special;
} __attribute__((packed));

static struct rx_desc *rx_desc;
static struct tx_desc *tx_desc;

enum REGISTER {
	CTRL = 0,
	STATUS = 8,
	EERD = 0x14,

	ICR = 0xC0,
	ICS = 0xC8,
	IMS = 0xD0,
	IMC = 0xD8,

	RX_CTRL = 0x100,
	TX_CTRL = 0x400,

	RX_DESC      = 0x2800,
	RX_DESC_LEN  = 0x2808,
	RX_DESC_HEAD = 0x2810,
	RX_DESC_TAIL = 0x2818,

	TX_DESC      = 0x3800,
	TX_DESC_LEN  = 0x3808,
	TX_DESC_HEAD = 0x3810,
	TX_DESC_TAIL = 0x3818,

	MTA = 0x5200,
	RAL = 0x5400,
	RAH = 0x5404
};

#define RX_CTRL_4K (3<<16 | 1<<25)

static u64 base_addr;

static void write_reg64(enum REGISTER reg, u64 value)
{
	*((u32 volatile *)(base_addr + reg+4)) = (u32)(value>>32);
	*((u32 volatile *)(base_addr + reg)) = (u32)value;
}

static void write_reg32(enum REGISTER reg, u32 value)
{
	*((volatile u32 *)(base_addr + reg)) = value;
}

static u32 read_reg32(enum REGISTER reg)
{
	return *((volatile u32 *)(base_addr + reg));
}

static void e1000_recv(void)
{
	unsigned int rx_cur;

	rx_cur = (read_reg32(RX_DESC_TAIL)+1)%RX_NUM;
	
	while(rx_desc[rx_cur].status & 0x1)
	{
		u64 buf;
		u16 len;

		buf = rx_desc[rx_cur].addr;
		len = rx_desc[rx_cur].len;

		net_give_packet(buf, len);
		rx_desc[rx_cur].status = 0;
		write_reg32(RX_DESC_TAIL, rx_cur);
		rx_cur++;
		rx_cur = rx_cur % RX_NUM;
	}
}

static void __attribute__((interrupt)) e1000_interrupt(void *p)
{

	(void)p;
	u32 icr;

	icr = read_reg32(ICR);
	if(icr & 0x80)
		e1000_recv();

	outb(0xA0, 0x20);
	outb(0x20, 0x20);
}


void e1000_send(uintptr_t addr, size_t len)
{
	static unsigned int tx_cur;

	tx_desc[tx_cur].addr = addr;
	tx_desc[tx_cur].len = len;
	tx_desc[tx_cur].cmd = 1 /* End of packet */ | 2 /* IFCS */;
	tx_desc[tx_cur].status = 0;

	tx_cur = (tx_cur+1)%TX_NUM;
	write_reg32(TX_DESC_TAIL, tx_cur);
	while(read_reg32(TX_DESC_HEAD) != tx_cur)
		;
}

static void e1000_tx_init(void)
{
	int i;

	tx_desc = kalloc();

	/* These get filled in by the hardware, just set status */
	for(i = 0; i < TX_NUM; i++)
	{
		tx_desc[i].addr = 0;
		tx_desc[i].status = 0;
		tx_desc[i].cmd = 0;
		tx_desc[i].status = 1;	/* For our use? */
	}

	write_reg64(TX_DESC, virt_to_phys(tx_desc));
	write_reg32(TX_DESC_LEN, sizeof (struct tx_desc[TX_NUM]));
	write_reg32(TX_DESC_HEAD, 0);
	write_reg32(TX_DESC_TAIL, TX_NUM-1);

	write_reg32(TX_CTRL, 1<<1 | 1<<3 | 0xF<<4 | 0x40<<12);
}

static void e1000_rx_init(void)
{
	int i;

	rx_desc = kalloc();

	/* Prime each RX descriptor with a 4k buffer */
	for(i = 0; i < RX_NUM; i++)
	{
		rx_desc[i].addr = net_get_buf();
		rx_desc[i].status = 0;
	}

	write_reg64(RX_DESC, virt_to_phys(rx_desc));
	write_reg32(RX_DESC_LEN, sizeof (struct rx_desc[RX_NUM]));
	write_reg32(RX_DESC_HEAD, 0);
	write_reg32(RX_DESC_TAIL, RX_NUM-1);

	/* Very promiscuous settings */
	write_reg32(RX_CTRL, 1<<1 | 1<<2 | 1<<3 |1<<4| 1<<5 | 1<<15 | RX_CTRL_4K);
}

void e1000_init(struct device *d)
{
	int i;
	u32 r;
	u8 mac[6];
	u32 command;

	command = device_read_u32(d, 0x4);
	command |= 0x4; /* PCI Bus mastering */
	device_write_u32(d, 0x4, command);

	base_addr = device_read_u32(d, 0x10);
	base_addr &= ~7;
	printf("[E1000] Base: %08x\n", base_addr);
	mmap(base_addr, base_addr, 0x10000, PT_WR | PT_PRESENT);

	/* Read the MAC address from the EEPROM */
	write_reg32(EERD, 1 | 0<<8);
	while(((r = read_reg32(EERD)) & 0x10) == 0);
	mac[5] = (r>>16)&0xFF;
	mac[4] = (r>>24)&0xFF;
	write_reg32(EERD, 1 | 1<<8);
	while(((r = read_reg32(EERD)) & 0x10) == 0);
	mac[3] = (r>>16)&0xFF;
	mac[2] = (r>>24)&0xFF;
	write_reg32(EERD, 1 | 2<<8);
	while(((r = read_reg32(EERD)) & 0x10) == 0);
	mac[1] = (r>>16)&0xFF;
	mac[0] = (r>>24)&0xFF;

	printf("[E1000] MAC: %02X%02X%02X%02X%02X%02X\n",
		mac[5], mac[4], mac[3], mac[2], mac[1], mac[0]
	);

	e1000_rx_init();
	e1000_tx_init();

	/* Clear Multicast Table Array */
	for(i = 0; i < 128; i++)
	;//	write_reg32(MTA + i*4, 0);

	/* Write the MAC back into the receive address register */
	write_reg32(RAL, mac[3]<<24 | mac[2]<<16 | mac[1]<<8 | mac[0]);
	write_reg32(RAH, mac[5]<<8 | mac[4]);
	write_reg32(IMC, ~0);
	read_reg32(ICR);

	/* Register ourself for IRQ11 */
	int_register(32+11, e1000_interrupt);

	/* Enable RX interrupt */
	write_reg32(ICR, 0xFFFF);
	write_reg32(IMS, 1<<7);
	write_reg32(ICS, 4);

	net_set_mac(mac);
}

