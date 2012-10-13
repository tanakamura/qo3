#include "kernel/uhci.h"
#include "kernel/pci.h"
#include "kernel/pci-id.h"
#include "kernel/wait.h"
#include "kernel/intrinsics.h"
#include <stdio.h>

static inline void w16(struct uhci_dev *dev,int reg,uint16_t val) {
	out16(dev->iobase + reg, val);
}
static inline void w8(struct uhci_dev *dev,int reg,uint8_t val) {
	out8(dev->iobase + reg, val);
}

static inline uint16_t r16(struct uhci_dev *dev,int reg) {
	return in16(dev->iobase + reg);
}
static inline uint8_t r8(struct uhci_dev *dev,int reg) {
	return in8(dev->iobase + reg);
}

int
uhci_init(struct pci_root *pci,
	  struct uhci_dev *dev,
	  struct uhci_init_error *error,
	  int pci_start)
{
	struct pci_device *p = pci->devices;
	int i, ret, n = pci->num_dev;

	for (i=pci_start; i<n; i++) {
		if (p[i].bcc == PCI_CLS_SERIAL_BUS &&
		    p[i].scc == PCI_SUBCLS_USB_HOST_CONTROLLER &&
		    p[i].pi == PCI_PI_UHCI) {
			break;
		}
	}

	if (i == n) {
		error->code = UHCI_INIT_NOT_FOUND;
		return -1;
	}
	ret = i;

	p = p+i;
	dev->iobase = pci_conf_read32(p, 0x20);

	if ((dev->iobase&1) == 0) {
		error->code = UHCI_INIT_IOADDR_NOT_FOUND;
		return -1;
	}

	dev->iobase = dev->iobase & ~(uintptr_t)1;

	uhci_dump(dev);

	w16(dev, 0, (1<<1));
	wait_msec(10);

	while (1) {
		if ((r16(dev, 0) & (1<<1)) == 0) {
			break;
		}
	}


	return ret;
}

void
uhci_dump(struct uhci_dev *dev)
{
	printf("addr base: %lx\n", dev->iobase);
	printf(" usbst: %x\n", (int)r16(dev,0));
}
