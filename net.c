#include "stdlib.h"
#include "types.h"
#include "net.h"
#include "print.h"
#include "e1000.h"
#include "stdlib.h"
#include "mem.h"

#define ARP_PACKET  0x0806
#define IPV4_PACKET 0x0800
#define IPV6_PACKET 0x86DD
#define ARP_REQ    0x1
#define PROTO_IPV4 0x0800
#define PROTO_ICMP 1
#define ICMP_REPLY 0
#define ICMP_ECHO 8

static unsigned char our_mac[6];

u16 htns(unsigned char *b)
{
	return (b[0]<<8) | b[1];
}

u32 htnl(unsigned char *b)
{
	return b[0]<<24 | b[1]<<16 | b[2]<<8 | b[3];
}


static void arp_send(unsigned char *dest_mac, unsigned char *source_mac,
	u32 dest_ip, u32 source_ip)
{
	unsigned char *packet;

	packet = kalloc();

	packet[ 0] = dest_mac[0];
	packet[ 1] = dest_mac[1];
	packet[ 2] = dest_mac[2];
	packet[ 3] = dest_mac[3];
	packet[ 4] = dest_mac[4];
	packet[ 5] = dest_mac[5];

	packet[ 6] = source_mac[0];
	packet[ 7] = source_mac[1];
	packet[ 8] = source_mac[2];
	packet[ 9] = source_mac[3];
	packet[10] = source_mac[4];
	packet[11] = source_mac[5];

	/* ARP */
	packet[12] = 0x08;
	packet[13] = 0x06;

	/* Ethernet */
	packet[14] = 0x00;
	packet[15] = 0x01;
	packet[16] = 0x08;
	packet[17] = 0x00;
	packet[18] = 0x06;
	packet[19] = 0x04;

	/* ARP reply */
	packet[20] = 0x00;
	packet[21] = 0x02;

	packet[22] = source_mac[0];
	packet[23] = source_mac[1];
	packet[24] = source_mac[2];
	packet[25] = source_mac[3];
	packet[26] = source_mac[4];
	packet[27] = source_mac[5];

	packet[28] = source_ip>>24;
	packet[29] = source_ip>>16;
	packet[30] = source_ip>>8;
	packet[31] = source_ip;

	packet[32] = dest_mac[0];
	packet[33] = dest_mac[1];
	packet[34] = dest_mac[2];
	packet[35] = dest_mac[3];
	packet[36] = dest_mac[4];
	packet[37] = dest_mac[5];

	packet[38] = dest_ip>>24;
	packet[39] = dest_ip>>16;
	packet[40] = dest_ip>>8;
	packet[41] = dest_ip;

	e1000_send(virt_to_phys(packet), 42);
	kfree(packet);
}

/* We are legion */
static void do_arp(unsigned char *buf, size_t len)
{
	u16 proto;
	u16 opcode;
	u32 source_ip, dest_ip;
	unsigned char source_mac[6];
	
	if(len < 42)
		return;

	proto = htns(&buf[16]);
	if(proto != PROTO_IPV4)
		return;

	opcode = htns(&buf[20]);
	if(opcode != ARP_REQ)
		return;

	memcpy(source_mac, &buf[22], 6);

	source_ip = htnl(&buf[28]);
	dest_ip = htnl(&buf[38]);

	arp_send(source_mac, our_mac, source_ip, dest_ip);
}

u32 checksum_calc(const u8 *data, size_t len)
{
	u32 sum = 0, i;

	for(i = 0; i < len; i += 2)
	{
		unsigned int n;

		n = data[i]<<8 | data[i+1];
		sum += n;
	}

	sum += (sum>>16)&0xFFFF;
	sum = ~sum;

	return sum & 0xFFFF;
}

static void icmp_reply(unsigned char *buf, size_t len)
{
	u8 *packet = kalloc();
	u32 sum;

	/* Ignore Ethernet checksum bytes */
	len -= 4;

	if(len < 42)
	{
		printf("ICMP packet too short\n");
		return;
	}

	if(buf[34] != 8)
	{
		printf("ICMP packet is not ECHO\n");
		return;
	}

	memcpy(packet, &buf[6], 6);
	memcpy(&packet[6], buf, 6);

	/* IPv4 */
	packet[12] = 0x8;
	packet[13] = 0;

	/* IP HEADER */
	/* ipv4 - 20bytes */
	packet[14] = 0x45;

	packet[15] = 0;

	packet[16] = 0;
	packet[17] = buf[17];

	packet[20] = 0;
	packet[21] = 0;

	/* ttl */
	packet[22] = 128;

	packet[23] = PROTO_ICMP;
	/* checksum */
	packet[24] = 0;
	packet[25] = 0;

	packet[26] = buf[26+4];
	packet[27] = buf[27+4];
	packet[28] = buf[28+4];
	packet[29] = buf[29+4];

	packet[30] = buf[26];
	packet[31] = buf[27];
	packet[32] = buf[28];
	packet[33] = buf[29];

	sum = checksum_calc(&packet[14], 20);
	packet[24] = sum>>8;
	packet[25] = sum&0xFF;

	/* ICMP */
	packet[34] = ICMP_REPLY;
	packet[35] = 0;
	/* chksum */
	packet[36] = 0x00;
	packet[37] = 0x00;

	packet[38] = buf[38];
	packet[39] = buf[39];

	packet[40] = buf[40];
	packet[41] = buf[41];

	memcpy(&packet[42], &buf[42], len-42);

	sum = checksum_calc(&packet[34], len-34);

	packet[36] = (sum>>8)&0xFF;
	packet[37] = sum&0xFF;

	e1000_send(virt_to_phys(packet), len);
	kfree(packet);
}

static void do_ipv4(unsigned char *buf, size_t len)
{
	if(buf[23] == PROTO_ICMP)
	{
		icmp_reply(buf, len);
	}
	else
	{
		printf("net: unknown ipv4 packet proto: %d\n", buf[23]);
	}
}

void net_give_packet(u64 buf, size_t len)
{
	u16 type;
	u8 *b = phys_to_virt(buf);

	if(len < 14)
		return;

	type = htns(&b[12]);
	switch(type)
	{
		case ARP_PACKET:
			do_arp(b, len);
		break;
		case IPV4_PACKET:
			do_ipv4(b, len);
		break;
		case IPV6_PACKET:
			printf("Net: ipv6 packet, skipping.\n");
		break;
		default:
			printf("Net: Unknown packet type %d\n", type);
			while(1);
	}
}

uintptr_t net_get_buf(void)
{
	return phys_alloc(PAGE_4K);
}

void net_set_mac(unsigned char *mac)
{
	memcpy(our_mac, mac, 6);
}

