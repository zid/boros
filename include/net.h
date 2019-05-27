#ifndef NET_H
#define NET_H
#include <types.h>
void net_give_packet(u64 buf, size_t len);
void net_set_mac(unsigned char *mac);
#endif
