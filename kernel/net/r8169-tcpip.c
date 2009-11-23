#include "kernel/net/r8169-tcpip.h"
#include "kernel/net/receiver.h"

void
r8169_tcpip_init(struct r8169_tcpip *tcpip,
		 struct r8169_dev *dev)
{
	tcpip->dev = dev;
	ndisc_init(&tcpip->ndisc);
	tcpip->rx_cur = 0;
	tcpip->rx_used = 0;
	tcpip->receivers = NULL;
}

int
r8169_tcpip_push_rx_buffer(struct r8169_tpcip *tcpip,
			   struct net_receiver *r,
			   int num_buffer,
			   void **buffers)
{
	int c=tcpip->rx_cur, u =tcpip->rx_used;
	int push_counter = 0;

	while (1) {
		int next;
		if (num_buffer <= 0) {
			break;
		}

		next = ((c+1)&R8169_TCPIP_RX_BUFFER_LEN_MASK);
		if (next == u) {
			break;
		}

		tcpip->rx_buffers[c] = *buffers;

		num_buffer--;
		buffers++;
		c = next;
	}

	tcpip->rx_cur = c;

	return push_counter;
}

