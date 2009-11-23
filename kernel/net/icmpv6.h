#ifndef QO3_KERNEL_NET_ICMPV6_H
#define QO3_KERNEL_NET_ICMPV6_H
#include <stdint.h>

#define icmpv6_build_header(type,code,chksum) ((chksum)<<16|(code)<<8|(type))

#define ICMPV6_RS_LENGTH 8
#define ICMPV6_RS_CHKSUM 0x85

static inline void
icmpv6_build_rs(void *packet, uint16_t pseudo_chksum)
{
	uint32_t *p = (uint32_t*)packet;
	p[0] = icmpv6_build_header(133, 0, pseudo_chksum);
	p[1] = 0;
}


#endif
