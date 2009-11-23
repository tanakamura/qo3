#ifndef QO3_KERNEL_UHCI_H
#define QO3_KERNEL_UHCI_H

#include <stdint.h>
#include "kernel/event.h"

enum uhci_init_error_code {
	UHCI_INIT_NOT_FOUND,
	UHCI_INIT_IOADDR_NOT_FOUND,
	UHCI_INIT_IRQ_NOT_FOUND
};

struct uhci_init_error {
	enum uhci_init_error_code code;
};

struct uhci_dev {
	uintptr_t iobase;
};


/* return negative if failed
 * return pciindex +1 if succeed.
 */
struct pci_root;
int uhci_init(struct pci_root *pci,
	      struct uhci_dev *dev,
	      struct uhci_init_error *error,
	      int pci_start);

void uhci_dump(struct uhci_dev *dev);

#endif
