#include <stdio.h>

#include "kernel/net/r8169.h"
#include "kernel/mmio.h"
#include "kernel/pci.h"
#include "kernel/pci-id.h"
#include "kernel/irq.h"
#include "kernel/net/mii.h"
#include "kernel/intrinsics.h"
#include "kernel/wait.h"
#include "kernel/config.h"
#include "kernel/atomic.h"

#define MAR0 0x08
#define Command 0x37
#define InterruptMask 0x3c
#define InterruptStatus 0x3e
#define PHYAR 0x60
#define RDSAR 0xe4
#define TNPDS 0x20
#define THPDS 0x28
#define PHYStatus 0x6c
#define RxConfig 0x44
#define TxConfig 0x40
#define RMS 0xda
#define TPPoll 0x38
#define MPC 0x4c
#define CR 0x50

#define OWN (1<<31)
#define EOR (1<<30)
#define FS (1<<29)
#define LS (1<<28)

static inline void
w8(struct r8169_dev *dev,int reg,uint8_t v) {
	mmio_write8(dev->mmio_base+reg, v);
}
static inline void
w16(struct r8169_dev *dev,int reg,uint16_t v) {
	mmio_write16(dev->mmio_base+reg, v);
}
static inline void
w32(struct r8169_dev *dev,int reg,uint32_t v) {
	mmio_write32(dev->mmio_base+reg, v);
}
static inline void
wAddr(struct r8169_dev *dev,int reg,uintptr_t v) {
	mmio_write32(dev->mmio_base+reg, v);
}

static inline uint8_t
r8(struct r8169_dev *dev,int reg) {
	return mmio_read8(dev->mmio_base+reg);
}
static inline uint16_t
r16(struct r8169_dev *dev,int reg) {
	return mmio_read16(dev->mmio_base+reg);
}
static inline uint32_t
r32(struct r8169_dev *dev,int reg) {
	return mmio_read32(dev->mmio_base+reg);
}
static inline uint32_t
rAddr(struct r8169_dev *dev,uintptr_t reg) {
	return mmio_read32(dev->mmio_base+reg);
}



static void
mdio_write(struct r8169_dev *dev, int phyreg, int val)
{
	w32(dev, PHYAR, 0x80000000 | (phyreg<<16) | val);

	while (1) {
		uint32_t val = r32(dev, PHYAR);
		if ((val & 0x80000000) == 0) {
			break;
		}

		//wait_usec(100);
	}
}

static uint16_t
mdio_read(struct r8169_dev *dev, int phyreg)
{
	w32(dev, PHYAR, phyreg<<16);

	while (1) {
		uint32_t val = r32(dev, PHYAR);
		if (val & 0x80000000) {
			return val;
		}

		wait_usec(100);
	}
}

static void
print_link(struct r8169_dev *dev)
{
	unsigned int phystat = r8(dev, PHYStatus);
	printf("link change: %08x\n", phystat);
}

static void
reset_state(struct r8169_dev *dev)
{
	int i;
	printf("%s:%d: 8169: %p\n",
	       __FILE__,
	       __LINE__,
	       dev);

	w16(dev, InterruptMask, 0x0); /* disable all interrupt */
	w16(dev, InterruptStatus, ~0); /* clear all interrupt */

	w8(dev, Command, 0); /* stop Tx&Rx */

	/* reset */
	w8(dev, Command, 1<<4);

	for (i=0; i<R8169_NUM_BUFFER; i++) {
		dev->tx_desc[i*4 + 0] = 0; /* bits */
		dev->tx_desc[i*4 + 1] = 0; /* vlan */
		dev->tx_desc[i*4 + 2] = 0; /* lo */
		dev->tx_desc[i*4 + 3] = 0; /* hi */

		dev->rx_desc[i*4 + 0] = 0; /* bits */
		dev->rx_desc[i*4 + 1] = 0; /* vlan */
		dev->rx_desc[i*4 + 2] = 0; /* lo */
		dev->rx_desc[i*4 + 3] = 0; /* hi */
	}
	printf("%s:%d: 8169: %p\n",
	       __FILE__,
	       __LINE__,
	       dev);

	dev->tx_desc[(R8169_NUM_BUFFER-1)*4] = EOR;
	dev->rx_desc[(R8169_NUM_BUFFER-1)*4] = EOR;

	dev->tx_pos = dev->tx_own_pos = 0;
	dev->rx_pos = dev->rx_own_pos = 0;
	dev->intr = 0;

	while (1) {
		if ((r8(dev,Command) & 1<<4) == 0)
			break;
		wait_usec(100);
	}

	w8(dev, CR, 0xc0);			  /* unlock */
	mdio_write(dev, MII_BMCR, 1<<15 | 1<<12); /* reset & auto negotiation*/

	w32(dev, TxConfig,
	    (r32(dev,TxConfig)&~(0x7<<8)) | /* ~0x7<<8 : clear DMA */
	    (0x7 << 8));	/* unlimited DMA */

	w32(dev, RxConfig,
	    /* fixme all */
	    0xe |		/* broadcast(8), multicast(4), physical match(2) */
	    (0x7 << 8) |	/* unlimited dma */
	    (0x7 << 13));	/* no threashold */

	/* fixme crc */
	w32(dev, MAR0, 0xffffffff);
	w32(dev, MAR0+4, 0xffffffff);

	w16(dev, RMS, (1<<14)-1);

	wAddr(dev, RDSAR, (uintptr_t)dev->rx_desc);
	wAddr(dev, TNPDS, (uintptr_t)dev->tx_desc);

	printf("%s:%d: 8169: %p\n",
	       __FILE__,
	       __LINE__,
	       dev);

	w8(dev, Command, (1<<2)|(1<<3)); /* Tx Enable */
	w16(dev, InterruptMask, 0xffef); /* enable all interrupt (RDU?) */
	w8(dev, CR, 0x0);		 /* lock */

	printf("%s:%d: 8169: %p\n",
	       __FILE__,
	       __LINE__,
	       dev);
}

static enum irq_handle_status
r8169_irq(int irq, void *data)
{
	struct r8169_dev *dev = (struct r8169_dev*)data;
	unsigned int bits = r16(dev, InterruptStatus);

	if (bits) {
		dev->intr++;
		if (bits & 0x804a) { /* 1000 0000 0100 1010 */
			w16(dev, InterruptStatus, 0x804a); /* accept error */
			printf("r8169 error: %x\n", bits & 0x804a);
		}

		if (bits & (1<<5)) { /* link change */
			w16(dev, InterruptStatus, (1<<5)); /* accept link change */
			print_link(dev);
		}

		if (bits & (1<<2)) { /* tx ok */
			int own = dev->tx_own_pos;
			int pos;
			uint32_t *desc = dev->tx_desc;
			event_bits_t **ep = dev->tx_event_pointers;
			event_bits_t *eb = dev->tx_event_bits;

			w16(dev, InterruptStatus, (1<<2)); /* accept rx ok */

			spinlock(dev->tx_lock); /* read tx_pos & OWN should be atomic */
			lfence();
			pos = dev->tx_pos;
			while (own != pos) {
				if (desc[own*4 + 0] & OWN) {
					break;
				}
				*ep[own] |= eb[own];
				own = (own+1) & R8169_DESC_POS_MASK;
			}
			spinunlock(dev->tx_lock);

			dev->tx_own_pos = own;
		}

		if (bits & (1<<0)) { /* rx ok */
			int own = dev->rx_own_pos;
			int pos;
			uint32_t *desc = dev->rx_desc;
			event_bits_t **ep = dev->rx_event_pointers;
			event_bits_t *eb = dev->rx_event_bits;
			uint32_t **flag_ret = dev->rx_flags_ret_ptr;

			w16(dev, InterruptStatus, 1); /* accept rx ok */

			spinlock(dev->rx_lock);
			lfence();
			pos = dev->rx_pos;
			while (own != pos) {
				if (desc[own*4 + 0] & OWN) {
					break;
				}
				*flag_ret[own] = desc[own*4+0];
				*ep[own] |= eb[own];
				own = (own+1) & R8169_DESC_POS_MASK;
			}
			spinunlock(dev->rx_lock);

			/* read desc -> update own pos should not be reorderd */
			mfence();

			dev->rx_own_pos = own;
		}

		return IRQ_HANDLED;
	}

	return IRQ_IGNORE;
}

int
r8169_init(struct pci_root *pci,
	   struct r8169_dev *dev,
	   struct r8169_init_error *error,
	   int pci_start)
{
	struct pci_device *p = pci->devices;
	int i, n = pci->num_dev;
	uintptr_t base = 0;
	int ret;
	const struct pci_irq_table *irq;

	for (i=pci_start; i<n; i++) {
		if (p[i].vendor_id == PCI_VENDOR_REALTEK) {
			if (p[i].device_id == 0x8136 ||
			    p[i].device_id == 0x8168) {
				/* fixme 64 */
				uint32_t memar = pci_conf_read32(p+i, 0x18);
				if (memar) {
					base = memar & 0xffffff00;
					if ((memar & 0xf3) == 0) {
						break;
					}
				}
			} else if (p[i].device_id == 0x8169) {
				uint32_t memar = pci_conf_read32(p+i, 0x14);
				base = memar & 0xffffff00;
				if ((memar & 0xff) == 0) {
					break;
				}
			}
		}
	}

	if (i == n) {
		error->code = R8169_INIT_NOT_FOUND;
		return -1;
	}
	ret = i;

	dev->mmio_base = base;

	for (i=0; i<6; i++) {
		dev->mac[i] = r8(dev, i);
	}

	irq = find_pci_irq_table(pci, &p[ret]);

	if (irq == NULL) {
		error->code = R8169_INIT_IRQ_NOT_FOUND;
		return -1;
	}

	dev->irq = irq;

	spinlock_init(&dev->tx_lock);
	spinlock_init(&dev->rx_lock);

	pci_irq_add(irq->irq[0], r8169_irq, dev);
	pci_irq_add(irq->irq[1], r8169_irq, dev);
	pci_irq_add(irq->irq[2], r8169_irq, dev);
	pci_irq_add(irq->irq[3], r8169_irq, dev);

	reset_state(dev);

	return ret;
}


enum r8169_enqueue_result
r8169_tx_packet(struct r8169_dev *dev,
		void *data_v, int n, int flags,
		event_bits_t *done_ptr, event_bits_t done_bits)
{
	unsigned char *data = data_v;
	int tx_pos = dev->tx_pos;
	uint32_t *d = dev->tx_desc;
	event_bits_t **ep = dev->tx_event_pointers;
	event_bits_t *eb = dev->tx_event_bits;
	uint32_t desc0;

	d += tx_pos * 4;

	if (d[0] & OWN) {
		return R8169_DESC_FULL;
	}

	d[2] = ((uintptr_t)data);

#ifdef ADDR64
	d[3] = ((uintptr_t)data)>>32;
#else
	d[3] = 0;
#endif

	/* should not be reordered.
	 * set buffer addr -> set OWN flag */
	sfence();

	ep[tx_pos] = done_ptr;
	eb[tx_pos] = done_bits;

	tx_pos ++;
	if (tx_pos == R8169_NUM_BUFFER) {
		desc0 = (n | OWN | FS | LS | EOR | flags);
		tx_pos = 0;
	} else {
		desc0 = (n | OWN | FS | LS | flags);
	}

	/* set desc + update pos should be atomic */
	spinlock_and_disable_int_self(dev->tx_lock);
	d[0] = desc0;
	dev->tx_pos = tx_pos;
	spinunlock_and_enable_int_self(dev->rx_lock);

	/* should not be reordered.
	 * set OWN flag -> polling */
	sfence();
	w8(dev, TPPoll, 1<<6);

	return R8169_ENQUEUE_OK;
}

enum r8169_enqueue_result
r8169_rx_packet(struct r8169_dev *dev,
		void *data_v, int n, uint32_t *flags,
		event_bits_t *done_ptr, event_bits_t done_bits)
{
	unsigned char *data = data_v;
	int rx_pos = dev->rx_pos;
	int rx_own_pos = dev->rx_own_pos;
	uint32_t *d = dev->rx_desc;
	event_bits_t **ep = dev->rx_event_pointers;
	event_bits_t *eb = dev->rx_event_bits;
	uint32_t **flags_ret = dev->rx_flags_ret_ptr;
	uint32_t desc0;

	d += rx_pos * 4;

	/*
	 *    rx_own_pos
	 *    v
	 * 01234567
	 *   ^
	 *   rx_pos
	 *
	 * (2 is empty, but to keep simple driver, don't use 2)
	 *
	 * rx_own_pos
	 * v
	 * 01234567
	 *        ^
	 *        rx_pos
	 *
	 * (7 is empty, but to keep simple driver, don't use 7)
	 *
	 * ((rx_pos+1)&7) = 0
	 *
	 */
	if (((rx_pos + 1)&R8169_DESC_POS_MASK) == rx_own_pos) {
		return R8169_DESC_FULL;
	}

	d[2] = ((uintptr_t)data);

#ifdef ADDR64
	d[3] = ((uintptr_t)data)>>32;
#else
	d[3] = 0;
#endif
	/* should not be reordered.
	 * set buffer -> set OWN */
	sfence();

	ep[rx_pos] = done_ptr;
	eb[rx_pos] = done_bits;
	flags_ret[rx_pos] = flags;

	rx_pos ++;
	if (rx_pos == R8169_NUM_BUFFER) {
		rx_pos = 0;
		desc0 = (n | OWN | EOR);
	} else {
		desc0 = (n | OWN);
	}

	/* set OWN & update rx_pos should be atomic */
	spinlock_and_disable_int_self(dev->rx_lock);
	d[0] = desc0;
	dev->rx_pos = rx_pos;
	spinunlock_and_enable_int_self(dev->rx_lock);

	w16(dev, InterruptStatus, 0x0010); /* clear RDU */

	return R8169_ENQUEUE_OK;
}


void
r8169_dump(struct r8169_dev *dev)
{
	int i;

	printf("r8169 irq: %d %d %d %d\n",
	       dev->irq->irq[0], dev->irq->irq[1], dev->irq->irq[2], dev->irq->irq[3]);
	printf("mmiobase : %lx\n", dev->mmio_base);
	printf("MAC : %02x %02x %02x %02x %02x %02x\n",
	       dev->mac[0], dev->mac[1], dev->mac[2], dev->mac[3], dev->mac[4], dev->mac[5]);

	printf("intr status: %08x\n", r16(dev, InterruptStatus));
	printf("intr mask: %08x\n", r16(dev, InterruptMask));
	printf("tx_pos:%d, rx_pos:%d\n", dev->tx_pos, dev->rx_pos);

	printf("mii control: %x\n", mdio_read(dev, MII_BMCR));
	printf("mii status: %x\n", mdio_read(dev, MII_BMSR));
	printf("phy status: %x\n", r8(dev, PHYStatus));
	printf("command: %08x\n", r8(dev, Command));
	printf("MPC(?): %08x\n", r32(dev, MPC));
	printf("intr: %d\n", dev->intr);

	for (i=0; i<R8169_NUM_BUFFER; i++) {
		printf("rx[%d]: %08x\n", i, dev->rx_desc[i*4]);
	}

	for (i=0; i<R8169_NUM_BUFFER; i++) {
		printf("tx[%d]: %08x\n", i, dev->tx_desc[i*4]);
	}

}

void
r8169_print_init_error(struct r8169_init_error *e)
{
	switch (e->code) {
	case R8169_INIT_NOT_FOUND:
		puts("device not found");
		break;

	case R8169_INIT_IRQ_NOT_FOUND:
		puts("irq not found");
		break;
	}
}
