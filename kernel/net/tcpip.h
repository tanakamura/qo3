#ifndef QO3_KERNEL_NET_TCPIP_H
#define QO3_KERNEL_NET_TCPIP_H

#include "kernel/net/ndisc.h"

#include <tmmintrin.h>
typedef unsigned char ipv6_addr_t[16];

#define TCPIP_LINK_ADDR_LINK_LOCAL 0
#define TCPIP_LINK_NUM_ADDR 4

struct __attribute__((aligned(16))) tcpip_link_state {
	ipv6_addr_t addrs[TCPIP_LINK_NUM_ADDR];
	__m128i mac;		/* (0,...0,mac[0],mac[1],...,mac[5]) */

	int num_addr;

	unsigned char mask[TCPIP_LINK_NUM_ADDR];

	struct ndisc_state ndisc;
};

enum tcpip_parse_result_code {
	TCPIP_INTERNAL,		/* consumed for internal use */
	TCPIP_REQUIRE_RESPONSE,	/* require response message */

	TCPIP_TOO_LARGE_PACKET,	      /* payload length == 0 */
	TCPIP_INVALID_PAYLOAD_LENGTH, /* payload length > frame length */

	TCPIP_ADDR_IGNORE,	/* not for me */

	TCPIP_IGNORE		/* ignored(error) */
};


enum tcpip_response_info_code {
	TCPIP_RESPONSE_ICMPv6_ECHO,
};

struct tcpip_response_info {
	enum tcpip_response_info_code code;
};

struct tcpip_parse_result {
	enum tcpip_parse_result_code code;

	union {
		struct tcpip_response_info response;
	} u;
};

void tcpip_parse_packet(struct tcpip_link_state *st,
			void *packet,
			int frame_len,
			struct tcpip_parse_result *result);

void tcpip_init(struct tcpip_link_state *st, unsigned char mac[6]);

void tcpip_dump(struct tcpip_link_state *st);

/* 14byte */
void tcpip_build_eth_header_multicast(void *packet,
				      const struct tcpip_link_state *st);
/* return frame size */
int tcpip_build_rs(void *packet,
		   const struct tcpip_link_state *st);

#endif
