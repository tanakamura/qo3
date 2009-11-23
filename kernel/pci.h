#ifndef QO3_KERNEL_PCI_H
#define QO3_KERNEL_PCI_H

#include <stdint.h>
#include "kernel/mmio.h"

enum pci_init_error_code {
	PCI_MCFG_NOT_FOUND,
	PCI_TOO_MANY_MCFG,
	PCI_MCFG_INVALID_BASE
};

struct pci_init_error {
	enum pci_init_error_code code;
};

struct pci_root;
/* return negative if failed */
int pci_init(struct pci_root *pci,
	     struct pci_init_error *error);

typedef uintptr_t busdevfn_t;
#define BDF_BUS(x) (((x)>>20)&0xff)
#define BDF_DEV(x) (((x)>>15)&0x1f)
#define BDF_FN(x) (((x)>>12)&0x07)
#define BDF_DEVFN(x) (((x)>>12)&0xff)

struct pci_device {
	busdevfn_t busdevfn;
	uint16_t vendor_id;
	uint16_t device_id;
	uint8_t scc;
	uint8_t bcc;
	uint8_t pi;
};

struct pci_irq_table {
	int addr;
	int irq[4];
};

struct pci_bridge {
	int devid;
	int busno;
	int num_irq_table;
	struct pci_irq_table *irq_table;
};

struct pci_root {
	uintptr_t mcfg_addr;
	int num_dev;
	struct pci_device *devices;
	int num_bridge;
	struct pci_bridge *bridges;

        /* brdid is offset of bridges
         *
         * | brdid | num_child | offset of | offset of | brdid  | num_child of |     |
         * | of    | of root   | child0    | child1    | of     | child0       | ... |
         * | root  | 2         | 5         | ...       | child0 | ...          |     |
         */
#define PCITREE_SIZEOF_NODE 2
#define PCITREE_OFFSET_BRIDGEID 0
#define PCITREE_OFFSET_NUM_CHILD 1

	int *tree;
};

extern struct pci_root pci_root0;

#if 1
static inline uint32_t
pci_conf_read32(struct pci_device *d, int reg)
{
	uintptr_t a = d->busdevfn + reg;
	return mmio_read32(a);
}
static inline uint16_t
pci_conf_read16(struct pci_device *d, int reg)
{
	uintptr_t a = d->busdevfn + reg;
	return mmio_read16(a);
}
static inline uint8_t
pci_conf_read8(struct pci_device *d, int reg)
{
	uintptr_t a = d->busdevfn + reg;
	return mmio_read8(a);
}
#else

#include "kernel/intrinsics.h"
/* use 0xcf8, 0xcfc pci controller (for bochs) */
#define BDF_TO_IOADDR(bdf)			\
	((BDF_BUS(bdf)<<16) |			\
	 (BDF_DEVFN(bdf)<<8))
	

static inline uint32_t
pci_conf_read32(struct pci_device *d, int reg)
{
	outl(0xcf8,
	     0x80000000 |
	     (BDF_BUS(d->busdevfn)<<16) |
	     (BDF_DEVFN(d->busdevfn)<<8) |
	     (reg & 0xfc));

	return inl(0xcfc);
}
static inline uint16_t
pci_conf_read16(struct pci_device *d, int reg)
{
	outl(0xcf8,
	     0x80000000 |
	     (BDF_BUS(d->busdevfn)<<16) |
	     (BDF_DEVFN(d->busdevfn)<<8) |
	     (reg & 0xfc));

	return inw(0xcfc + (reg&2));
}
static inline uint8_t
pci_conf_read8(struct pci_device *d, int reg)
{
	outl(0xcf8,
	     0x80000000 |
	     (BDF_BUS(d->busdevfn)<<16) |
	     (BDF_DEVFN(d->busdevfn)<<8) |
	     (reg & 0xfc));

	return inb(0xcfc + (reg&3));
}
#endif

static inline uintptr_t
pci_bdf_to_addr(struct pci_root *pci,
		unsigned int bus,
		unsigned int dev,
		unsigned int fn)
{
	uintptr_t ret = pci->mcfg_addr;
	ret |= bus<<20;
	ret |= dev<<15;
	ret |= fn<<12;
	return ret;
}

static inline uint32_t
pci_conf_read32_bdf(struct pci_root *pci,
		    unsigned int bus,
		    unsigned int dev,
		    unsigned int fn,
		    int reg)
{
	uintptr_t a = pci_bdf_to_addr(pci, bus,dev,fn) + reg;
	return mmio_read32(a);
}
static inline uint16_t
pci_conf_read16_bdf(struct pci_root *pci,
		    unsigned int bus,
		    unsigned int dev,
		    unsigned int fn,
		    int reg)
{
	uintptr_t a = pci_bdf_to_addr(pci, bus,dev,fn) + reg;
	return mmio_read16(a);
}
static inline uint8_t
pci_conf_read8_bdf(struct pci_root *pci,
		   unsigned int bus,
		   unsigned int dev,
		   unsigned int fn,
		   int reg)
{
	uintptr_t a = pci_bdf_to_addr(pci, bus,dev,fn) + reg;
	return mmio_read8(a);
}

struct pci_device *find_pci(int vendor_id, int device_id);

/* returns NULL if failed */
const struct pci_irq_table *find_pci_irq_table(struct pci_root *root,
					       struct pci_device *dev);


void lspci(struct pci_root *pci);
void lspci_tree(struct pci_root *pci);

#endif
