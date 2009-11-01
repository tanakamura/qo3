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

struct pci_device {
	busdevfn_t busdevfn;
	uint16_t vendor_id;
	uint16_t device_id;
	uint8_t scc;
	uint8_t bcc;
};

struct pci_irq_table {
	int addr;
	int irq[4];
};

struct pci_bridge {
	int devid;
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
	 * | brdid | busno   | num_child | offset of | offset of | brdid  | busno | num_child of |     |
	 * | of	   | of root | of root	 | child0    | child1	 | of	  | of	  | child0	 | ... |
	 * | root  | 0	     | 2	 | 5	     | ...	 | child0 | c0	  | ...		 |     |
	 */
#define PCITREE_SIZEOF_NODE 3
#define PCITREE_OFFSET_BRIDGEID 0
#define PCITREE_OFFSET_BUSNO 1
#define PCITREE_OFFSET_NUM_CHILD 2

	int *tree;
};

extern struct pci_root pci_root0;

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
void lspci(struct pci_root *pci);
void lspci_tree(struct pci_root *pci);

#endif
