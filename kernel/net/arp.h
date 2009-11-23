#ifndef QO3_KERNEL_NET_ARP_H
#define QO3_KERNEL_NET_ARP_H

struct arp4_cache {
	int cur;
	uint32_t buffers[32];
};


#endif
