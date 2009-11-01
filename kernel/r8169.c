#include "kernel/r8169.h"
#include "kernel/mmio.h"
#include "kernel/pci.h"
#include "kernel/pci-id.h"
#include <stdio.h>
#include "kernel/wait.h"

#define RDSAR 0xe4

static inline void
w8(struct r8169_dev *dev,int reg,uint8_t v)
{
	mmio_write8(dev->mmio_base+reg, v);
}
static inline void
w16(struct r8169_dev *dev,int reg,uint16_t v)
{
	mmio_write16(dev->mmio_base+reg, v);
}
static inline void
w32(struct r8169_dev *dev,int reg,uint32_t v)
{
	mmio_write32(dev->mmio_base+reg, v);
}

static inline uint8_t
r8(struct r8169_dev *dev,int reg)
{
	return mmio_read8(dev->mmio_base+reg);
}

static inline uint16_t
r16(struct r8169_dev *dev,int reg)
{
	return mmio_read16(dev->mmio_base+reg);
}
static inline uint32_t
r32(struct r8169_dev *dev,int reg)
{
	return mmio_read32(dev->mmio_base+reg);
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

	for (i=pci_start; i<n; i++) {
		if (p[i].vendor_id == PCI_VENDOR_REALTEK) {
			if (p[i].device_id == 0x8136 ||
			    p[i].device_id == 0x8168) {
				/* fixme 64 */
				uint32_t memar = pci_conf_read32(p+i, 0x18);
				printf("%x\n", memar);

				if (memar) {
					base = memar & 0xffffff00;
					if ((memar & 0xfb) == 0) {
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

	/* reset */
	w8(dev, 0x37, 1<<4);
	while (1) {
		if ((r8(dev,0x37) & 1<<4) == 0)
			break;
		wait_usec(100);
	}

	for (i=0; i<6; i++) {
		dev->mac[i] = r8(dev, i);
		printf("%x\n", dev->mac[i]);
	}

	return ret;
}
