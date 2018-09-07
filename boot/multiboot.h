struct syms
{
	u32 num, size, addr, shndx;
};

struct mem
{
	u32 size;
	u64 addr;
	u64 len;
	u32 type;
};

struct module
{
	u32 mod_start;
	u32 mod_end;
	const char *name;
	char reserved[12];
};

struct multiboot
{
	u32 flags;
	u32 mem_lower, mem_upper;
	u32 boot_device;
	u32 cmdline;
	u32 mods_count;
	struct module *mods_addr;
	struct syms syms;
	u32 mmap_len;
	struct mem *mmap_addr;
};

