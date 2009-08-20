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

/* return negative if failed */
int pci_init(struct pci_init_error *error);

typedef uintptr_t busdevfn_t;
struct pci_device {
	busdevfn_t busdevfn;
	uint16_t vendor_id;
	uint16_t device_id;
};

extern int pci_num_dev;
extern struct pci_device *pci_devices;


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

extern uintptr_t pci_mcfg_addr;

static inline uintptr_t
pci_bdf_to_addr(unsigned int bus,
		unsigned int dev,
		unsigned int fn)
{
	uintptr_t ret = pci_mcfg_addr;
	ret |= bus<<20;
	ret |= dev<<15;
	ret |= fn<<12;
	return ret;
}

static inline uint32_t
pci_conf_read32_bdf(unsigned int bus,
		    unsigned int dev,
		    unsigned int fn,
		    int reg)
{
	uintptr_t a = pci_bdf_to_addr(bus,dev,fn) + reg;
	return mmio_read32(a);
}
static inline uint16_t
pci_conf_read16_bdf(unsigned int bus,
		    unsigned int dev,
		    unsigned int fn,
		    int reg)
{
	uintptr_t a = pci_bdf_to_addr(bus,dev,fn) + reg;
	return mmio_read16(a);
}
static inline uint8_t
pci_conf_read8_bdf(unsigned int bus,
		   unsigned int dev,
		   unsigned int fn,
		   int reg)
{
	uintptr_t a = pci_bdf_to_addr(bus,dev,fn) + reg;
	return mmio_read8(a);
}



struct pci_device *find_pci(int vendor_id, int device_id);

#endif
