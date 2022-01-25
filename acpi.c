#include <types.h>
#include <mem.h>
#include <print.h>
#include <stdlib.h>
#include <pci.h>

struct rsd_desc
{
	char sig[8];
	u8 checksum;
	char oem[6];
	u8 rev;
	u32 rsdt_addr;
};

struct sdt_header
{
	char sig[4];
	u32  len;
	u8   rev;
	u8   checksum;
	char oem_id[6];
	char oem_table_id[8];
	u32 oem_rev;
	u32 creator_id;
	u32 creator_rev;
};

struct mcfg
{
	struct sdt_header sdt;
	u32 resv, resv2;

	struct {
		u64 base;
		u16 group;
		u8 start, end;
	} m[];
} __attribute((packed));

struct madt
{
	struct sdt_header sdt;

	u32 apic_addr;
	u32 flags;
} __attribute((packed));

struct rsdt
{
	struct sdt_header sdt;
	u32 ptrs[];
};


/*
 * SEABIOS at least does not align these tables, let the compiler figure out
 * if unaligned reads are allowed on this platform.
 */

static u32 u32_at(unsigned char *p)
{
	u64 v;

	v = 0<<8 | p[3];
	v = v<<8 | p[2];
	v = v<<8 | p[1];
	v = v<<8 | p[0];

	return v;
}

static u64 u64_at(unsigned char *p)
{
	u64 v;

	v = 0<<8 | p[7];
	v = v<<8 | p[6];
	v = v<<8 | p[5];
	v = v<<8 | p[4];
	v = v<<8 | p[3];
	v = v<<8 | p[2];
	v = v<<8 | p[1];
	v = v<<8 | p[0];

	return v;
}
static int acpi_checksum_valid(void *m, u8 checksum, size_t len)
{
	u8 sum = 0;
	u8 *p = m;

	size_t i;

	for(i = 0; i < len; i++)
		sum += p[i];

	return sum == checksum;
}

static void acpi_parse_mcfg(struct sdt_header *sdt)
{
	struct mcfg *m = (struct mcfg *)sdt;
	size_t i, max;

	if(acpi_checksum_valid(m, m->sdt.checksum, m->sdt.len) != 0)
	{
		printf("\tMCFG checksum invalid\n");
		return;
	}

	/* MCFG header is 44 bytes, each entry is 16 */
	max = (m->sdt.len - 44) / 16;

	for(i = 0; i < max; i++)
	{
		printf("\tBase: %lx\n", m->m[0].base);
		printf("\tStart: %x\n", m->m[0].start);
		printf("\tEnd: %x\n", m->m[0].end);

		pcie_init(m->m[0].base);
	}
}

static void acpi_parse_madt(struct sdt_header *sdt)
{
	struct madt *m = (struct madt *)sdt;
	unsigned char *p;
	size_t len;

	if(acpi_checksum_valid(m, m->sdt.checksum, m->sdt.len) != 0)
	{
		printf("\tMADT checksum invalid\n");
		return;
	}

	/* Length of data appended to MADT */
	len = (m->sdt.len - sizeof (struct madt));
	p = (unsigned char *)(m + 1);

	while(1)
	{
		int record_type, record_len;

		record_type = p[0];
		record_len  = p[1];

		switch(record_type)
		{
			case 0: /* LAPIC */
			printf("\tAPIC Processor ID: 0x%02X\n", p[2]);
			printf("\tAPIC ID: 0x%02X\n", p[3]);
			break;
			case 1: /* IOAPIC */
			printf("\tIOAPIC ID: 0x%02X\n", p[2]);
			printf("\tIOAPIC Base: 0x%08X\n", u32_at(p+4));
			break;
			case 2: /* IOAPIC Interrupt Source */
			printf("\tIOAPIC Interrupt Source\n");
			break;
			case 5: /* LAPIC Override */
			printf("\tLAPIC 64-bit Base: 0x%016lX\n", u64_at(p+4));
			break;
		}

		p += record_len;
		if(p >= (unsigned char *)(m + 1) + len)
			break;
	}
}

static void acpi_table_parse(u32 ptr)
{
	struct sdt_header *sdt = (void *)phys_to_virt(ptr);

	printf("ACPI table at %x\n", ptr);
	mmap((u64)sdt, ptr, 0x1000, PT_WR | PT_PRESENT);
	printf("\tType: %c%c%c%c\n",
		sdt->sig[0],
		sdt->sig[1],
		sdt->sig[2],
		sdt->sig[3]
	);

	if(memcmp(sdt->sig, "MCFG", 4) == 0)
		acpi_parse_mcfg(sdt);

	if(memcmp(sdt->sig, "APIC", 4) == 0)
		acpi_parse_madt(sdt);

	return;
}

static void acpi_walk_rsdt(u32 rsdt_addr)
{
	struct rsdt *r;
	int invalid;
	size_t i;

	r = (struct rsdt *)phys_to_virt(rsdt_addr);

	/* Two pages in case it straddles */
	mmap((u64)r, rsdt_addr & ~0xFFF, 0x2000, PT_WR | PT_PRESENT);

	if(r->sdt.len > 0x1000)
	{
		printf("ACPI RSDT too long!\n");
		return;
	}

	invalid = acpi_checksum_valid(r, r->sdt.checksum, r->sdt.len);
	if(invalid)
	{
		printf("Checksum for ACPI RSDT doesn't match\n");
		return;
	}

	for(i = 0; i < (r->sdt.len - sizeof(*r)) / 4; i++)
	{
		acpi_table_parse(r->ptrs[i]);
	}

	/* TODO: munmap */
}

static int rsd_read(struct rsd_desc *rd)
{
	int invalid;

	invalid = acpi_checksum_valid(rd, rd->checksum, sizeof(*rd));
	if(invalid)
		return 1;

	printf("ACPI RSDP table found at: 0x%08x\n", (u32)(u64)rd);
	if(rd->rev != 0)
	{
		printf("Didn't understand ACPI revision %d\n", rd->rev);
		return 1;
	}

	printf("\tOEM: %c%c%c%c%c%c\n",
		rd->oem[0],
		rd->oem[1],
		rd->oem[2],
		rd->oem[3],
		rd->oem[4],
		rd->oem[5]
	);

	printf("\tRevision: %d\n", rd->rev);
	printf("\tAddress: %08X\n", rd->rsdt_addr);

	acpi_walk_rsdt(rd->rsdt_addr);

	return 0;
}

void acpi_init(void)
{
	u64 magic = 0x2052545020445352UL;
	u64 *p;
	u32 offset;

	printf("ACPI mapping...\n");

	mmap((u64)phys_to_virt(0xE0000), 0xE0000, 128*1024, PT_WR | PT_PRESENT);

	p = (void *)phys_to_virt(0xE0000);

	for(offset = 0; offset < 128*1024; offset += 8, p++)
	{
		if(*p != magic)
			continue;

		if(rsd_read((struct rsd_desc *)p) == 0)
			return;
	}

	/* TODO: munmap */
}
