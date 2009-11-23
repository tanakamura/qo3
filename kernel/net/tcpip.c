#include "kernel/net/tcpip.h"
#include "kernel/net/icmpv6.h"
#include "kernel/net/ethernet.h"
#include "kernel/intrinsics.h"
#include "kernel/sse-mapping.h"
#include <stdint.h>
#include <stdio.h>

void
tcpip_parse_packet(struct tcpip_link_state *st,
		   void *packet_v,
		   int frame_len,
		   struct tcpip_parse_result *result)
{
	const unsigned char *p = (unsigned char*)packet_v;
	uint16_t pay_len = movbe_load16((uint16_t*)(p + 4));
	__m128i from, to;
	int for_me, i, n;

	if (pay_len == 0) {
		result->code = TCPIP_TOO_LARGE_PACKET;
		return ;
	}

	if (pay_len > (frame_len - 40)) {
		result->code = TCPIP_INVALID_PAYLOAD_LENGTH;
		return ;
	}

	if (*p == 0xff)		/* multicast */
		goto mine;

	from = _mm_loadu_si128((__m128i*)(p+8));
	to = _mm_loadu_si128((__m128i*)(p+8+16));

	n = st->num_addr;
	for (i=0; i<n; i++) {
		__m128i a = _mm_load_si128((__m128i*)&st->addrs[i]);
		for_me = _mm_movemask_epi8(_mm_cmpeq_epi8(from, a));
		if (for_me == 0xffff)
			goto mine;
	}
	result->code = TCPIP_ADDR_IGNORE;
	return;

mine:
	puts("its mine");
}

static const unsigned char LINK_LOCAL_MULTICAST[] = {
	0xff,0x02,
	0x00,0x00,
	0x00,0x00,
	0x00,0x00,
	0x00,0x00,
	0x00,0x00,
	0x00,0x00,
	0x00,0x02,
};

#define ADDR_ADD(t, p, b) ((t*)(((uintptr_t)(p)) + (b)))
#define IPV6_HEADER_LEN (8 + 16*2)

void tcpip_build_eth_header_multicast_ipv6(void *packet,
					   const struct tcpip_link_state *st)
{
	__m128i template = _mm_set_epi8(0x00, 0x00,
					0xdd, 0x86, /* ipv6 */
					0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
					0x02, 0x00, 0x00, 0x00, 0x33, 0x33);
	__m128i mac_shifted = _mm_srli_si128(st->mac, 4);
	__m128i header = _mm_or_si128(template, mac_shifted);

	((__m128i*)packet)[0] = header;
}

static inline __m128i
le2be_16(__m128i x)
{
	return _mm_shuffle_epi8(x, _mm_set_epi8(0xe, 0xf, 0xc, 0xd,
						0xa, 0xb, 0x8, 0x9,
						0x6, 0x7, 0x4, 0x5,
						0x2, 0x3, 0x0, 0x1));
}

static uint16_t
chksum_pseudo_header(const ipv6_addr_t from,
		     const ipv6_addr_t to,
		     unsigned int len,
		     unsigned int next,
		     uint32_t sum)
{
	__asm__ ("addl	0(%1), %0\n\t"
		 "adcl	4(%1), %0\n\t"
		 "adcl	8(%1), %0\n\t"
		 "adcl	12(%1), %0\n\t"
		 "adcl	0(%2), %0\n\t"
		 "adcl	4(%2), %0\n\t"
		 "adcl	8(%2), %0\n\t"
		 "adcl	12(%2), %0\n\t"
		 "adcl	%3, %0\n\t"
		 "adcl	%4, %0\n\t"
		 "adcl	$0, %0\n\t"
		 :"+r"(sum)
		 :"r"(from), "r"(to),
		  "g"(len), "g"((uint32_t)next));

	sum = (sum&0xffff) + (sum>>16);
	sum = (sum&0xffff) + (sum>>16);

	return sum;
}

int
tcpip_build_rs(void *packet,
	       const struct tcpip_link_state *st)
{
	uint32_t *p32;
	uint16_t sum16;
	void *payload;
	__m128i *p128;
	__m128i la, llm;

	tcpip_build_eth_header_multicast_ipv6(packet, st);

	p32 = ADDR_ADD(uint32_t, packet, ETH_HEADER_LEN);

	/*                      ver           traf     flow */
	movbe_store32(&p32[0], (6<<(12+16)) | (0+16) | 0);
	movbe_store32(&p32[1], (ICMPV6_RS_LENGTH<<16) | (58<<8) | 255);

	p128 = ADDR_ADD(__m128i, packet, ETH_HEADER_LEN+8);

	la = _mm_load_si128((__m128i*)st->addrs[TCPIP_LINK_ADDR_LINK_LOCAL]);
	llm = _mm_load_si128((__m128i*)LINK_LOCAL_MULTICAST);

	_mm_storeu_si128(((__m128i*)p128), la);
	_mm_storeu_si128(((__m128i*)p128)+1, llm);

	sum16 = ~chksum_pseudo_header(st->addrs[TCPIP_LINK_ADDR_LINK_LOCAL],
				      LINK_LOCAL_MULTICAST,
				      ICMPV6_RS_LENGTH<<24,
				      58<<24, ICMPV6_RS_CHKSUM);

	payload = ADDR_ADD(void, packet, (IPV6_HEADER_LEN+ETH_HEADER_LEN));
	icmpv6_build_rs(payload, sum16);

	return (ETH_HEADER_LEN + IPV6_HEADER_LEN + ICMPV6_RS_LENGTH);
}


void
tcpip_init(struct tcpip_link_state *st, unsigned char mac[6])
{
	__m128i la = _mm_set_epi8(mac[5], mac[4], mac[3],
				  0xfe, 0xff,
				  mac[2], mac[1], mac[0],
				  0, 0, 0, 0,
				  0, 0, 0x80, 0xfe);

	st->mac = _mm_set_epi8(mac[5], mac[4], mac[3], mac[2], mac[1], mac[0], 0, 0,
			       0, 0, 0, 0, 0, 0, 0, 0);
	_mm_store_si128((__m128i*)st->addrs[TCPIP_LINK_ADDR_LINK_LOCAL], la);
	st->mask[TCPIP_LINK_ADDR_LINK_LOCAL] = 64;
	st->num_addr = 1;

	ndisc_init(&st->ndisc);
}

void
tcpip_dump(struct tcpip_link_state *st)
{
	int i;
	for (i=0; i<st->num_addr; i++) {
		__m128i a;
		a = _mm_load_si128((__m128i*)st->addrs[i]);
		a = _mm_shuffle_epi8(a, _mm_set_epi8(0xe, 0xf, 0xc, 0xd,
						     0xa, 0xb, 0x8, 0x9,
						     0x6, 0x7, 0x4, 0x5,
						     0x2, 0x3, 0x0, 0x1));

		printf("%x:", (uint16_t)_mm_extract_epi16(a, 0));
		printf("%x:", (uint16_t)_mm_extract_epi16(a, 1));
		printf("%x:", (uint16_t)_mm_extract_epi16(a, 2));
		printf("%x:", (uint16_t)_mm_extract_epi16(a, 3));
		printf("%x:", (uint16_t)_mm_extract_epi16(a, 4));
		printf("%x:", (uint16_t)_mm_extract_epi16(a, 5));
		printf("%x:", (uint16_t)_mm_extract_epi16(a, 6));
		printf("%x", (uint16_t)_mm_extract_epi16(a, 7));
		printf("/%d\n", st->mask[i]);
	}
}
