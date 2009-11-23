#include "kernel/net/receiver.h"

void
net_receiver_init(struct net_receiver *r,
		  event_bits_t *notify_ptr,
		  event_bits_t notify_bits);

{
	r->counter = 0;
	r->notify_ptr = notify_ptr;
	r->notify_bits = notify_bits;
}
