#ifndef QO3_KERNEL_NET_NDISC_H
#define QO3_KERNEL_NET_NDISC_H

struct ndisc_state {
	int buffer[32];
};

void ndisc_init(struct ndisc_state *ndisc);

#endif
