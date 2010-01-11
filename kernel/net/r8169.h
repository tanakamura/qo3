#ifndef QO3_KERNEL_R8169_H
#define QO3_KERNEL_R8169_H

#include <stdint.h>
#include "kernel/event.h"
#include "kernel/atomic.h"

enum r8169_init_error_code {
	R8169_INIT_NOT_FOUND,
	R8169_INIT_IRQ_NOT_FOUND
};

struct r8169_init_error {
	enum r8169_init_error_code code;
};

void r8169_print_init_error(struct r8169_init_error *e);

#define R8169_NUM_BUFFER 16
#define R8169_DESC_POS_MASK 0xf

struct __attribute__((aligned(256))) r8169_dev {
	uint32_t rx_desc[R8169_NUM_BUFFER * 4] __attribute__((aligned(256)));
	uint32_t tx_desc[R8169_NUM_BUFFER * 4] __attribute__((aligned(256)));

	event_bits_t *tx_event_pointers[R8169_NUM_BUFFER];
	event_bits_t tx_event_bits[R8169_NUM_BUFFER];

	event_bits_t *rx_event_pointers[R8169_NUM_BUFFER];
	event_bits_t rx_event_bits[R8169_NUM_BUFFER];
	uint32_t *rx_flags_ret_ptr[R8169_NUM_BUFFER];

	int rx_pos;
	int rx_own_pos;
	int tx_pos;
	int tx_own_pos;
	int intr;

	uintptr_t mmio_base;
	unsigned char mac[6];
	const struct pci_irq_table *irq;

	spinlock_t tx_lock;
	spinlock_t rx_lock;
};

/* return negative if failed
 * return pciindex +1 if succeed.
 */
struct pci_root;
int r8169_init(struct pci_root *pci,
	       struct r8169_dev *dev,
	       struct r8169_init_error *error,
	       int pci_start);

void r8169_dump(struct r8169_dev *dev);

enum r8169_enqueue_result {
	R8169_ENQUEUE_OK,
	R8169_DESC_FULL
};

#define R8169_TX_IP_CHKSUM (1<<18)
#define R8169_TX_UDP_CHKSUM (1<<17)
#define R8169_TX_TCP_CHKSUM (1<<16)

enum r8169_enqueue_result r8169_tx_packet(struct r8169_dev *dev,
					  void *data, int n, int flags,
					  event_bits_t *done_ptr, event_bits_t done_bits);
enum r8169_enqueue_result r8169_rx_packet(struct r8169_dev *dev,
					  void *data, int n, uint32_t *flags,
					  event_bits_t *ready_ptr, event_bits_t ready_bits);

#define R8169_RX_COMPLETE(flag) (((flag)&OWN) == 0)
#define R8169_RX_CLEAR_COMPLETE(flag) ((flag) = ~0)

#endif
