#ifndef QO3_KERNEL_NET_R8169_TCPIP_H
#define QO3_KERNEL_NET_R8169_TCPIP_H

#include "kernel/net/r8169.h"
#include "kernel/net/ndisc.h"

#define R8169_TCPIP_RX_BUFFER_LEN 1024
#define R8169_TCPIP_RX_BUFFER_LEN_MASK (1023)

struct r8169_tcpip {
	struct r8169_dev *dev;
	struct net_receiver *receivers;

	void *rx_buffers[R8169_TCPIP_RX_BUFFER_LEN];
	uint32_t flags[R8169_TCPIP_RX_BUFFER_LEN];

	int rx_cur;
	int rx_used;

	event_bits_t event_bits;
};

struct net_receiver;

#define R8169_TCPIP_BUFSIZE 4096

void r8169_tcpip_init(struct r8169_tcpip *tcpip,
		      struct r8169_dev *dev);

void r8169_tcpip_fini(struct r8169_tcpip *tcpip);

struct r8169_tcpip_rx_result {
	void *buf_addr;
	uint32_t flags;
};

void r8169_tcpip_rx(struct r8169_tcpip *tcpip,
		    struct net_receiver *r,
		    struct r8169_tcpip_rx_result *results);

void r8169_tcpip_run_rx(struct r8169_tcpip *tcpip);

int r8169_tcpip_push_rx_buffer(struct r8169_tpcip *tcpip,
			       struct net_receiver *r,
			       int num_buffer,
			       void **buffers);

void r8169_tcpip_start_rx(struct r8169_tcpip *tcpip,
			  struct net_receiver *r);
void r8169_tcpip_stop_rx(struct r8169_tcpip *tcpip,
			 struct net_receiver *r);

#endif
