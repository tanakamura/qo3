#ifndef QO3_KERNEL_R8169_H
#define QO3_KERNEL_R8169_H

#include <stdint.h>

enum r8169_init_error_code {
	R8169_INIT_NOT_FOUND
};
struct r8169_init_error {
	enum r8169_init_error_code code;
};

struct r8169_dev {
	uintptr_t mmio_base;
	unsigned char mac[6];
};

/* return negative if failed
 * return pciindex +1 if succeed.
 */
struct pci_root;
int r8169_init(struct pci_root *pci,
	       struct r8169_dev *dev,
	       struct r8169_init_error *error,
	       int pci_start);

#endif
