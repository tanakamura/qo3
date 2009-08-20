#include "kernel/pci.h"
#include "kernel/qo3-acpi.h"
#include "kernel/mmio.h"
#include <stdio.h>
#include "kernel/brk.h"

int pci_num_dev;
struct pci_device *pci_devices;
uintptr_t pci_mcfg_addr;

/*
 * 36 header
 * 8  reserved(?)
 * 44 allocation
 * 
 * 16byte:
 *   0-7 address
 *   8-9 pci_segment
 *   10 start_bus
 *   11 end_bus
 *   12-15 reserved
 */

int
pci_init(struct pci_init_error *error)
{
	uintptr_t mcfg = find_acpi_description_entry(ACPI_SIG('M','C','F','G'));
	unsigned int len;
	unsigned int num_entry;
	uintptr_t entry;
	uintptr_t address;
	uintptr_t start, end, bus;
	int num_dev;
	int cur_dev;
	
	if (!mcfg) {
		error->code = PCI_MCFG_NOT_FOUND;
		return -1;
	}

	len = ACPI_R32(mcfg, 4);
	len -= 44;
	num_entry = len/16;

/*
	for (i=0; i<num_entry; i++) {
		uintptr_t entry = mcfg + 44 + i*16;
		printf("address = %x\n", ACPI_R32(entry, 0));
		printf("segment = %x\n", ACPI_R16(entry, 8));
		printf("start = %d\n", ACPI_R8(entry, 10));
		printf("end = %d\n", ACPI_R8(entry, 11));
	}
*/

	if (num_entry != 1) {
		error->code = PCI_TOO_MANY_MCFG;
		return -1;
	}

	entry = mcfg + 44;
	address = ACPI_R64ADDR(entry, 0);
	if (address & ~0xf0000000) {
		error->code = PCI_MCFG_INVALID_BASE;
		return -1;
	}

	pci_mcfg_addr = address;

	start = ACPI_R8(entry, 10);
	end = ACPI_R8(entry, 11);

	start <<= 20;
	end <<= 20;

	num_dev = 0;

	for (bus=start; bus<end; bus+=(1<<20)) {
		uintptr_t dev;
		for (dev=0; dev<(32<<15); dev += (1<<15)) {
			uintptr_t mmio_addr = address | bus | dev;
			uint32_t id = mmio_read32(mmio_addr);
			uint32_t header;
			if (id == 0xffffffff || id == 0) {
				continue;
			}
			num_dev++;

			header = mmio_read32(mmio_addr + 0x0E);
			if (header & 0x80) {
				uintptr_t fn;
				for (fn = 0; fn<(8<<12); fn+=(1<<12)) {
					mmio_addr = address | bus | dev | fn;
					id = mmio_read32(mmio_addr);
					if (id == 0xffffffff || id == 0) {
						continue;
					}

					num_dev++;
				}
			}
		}
	}

	pci_devices = SBRK_TA4(struct pci_device, num_dev);
	pci_num_dev = num_dev;
	cur_dev = 0;

	for (bus=start; bus<end; bus+=(1<<20)) {
		uintptr_t dev;
		for (dev=0; dev<(32<<15); dev += (1<<15)) {
			uintptr_t mmio_addr = address | bus | dev;
			uint32_t id = mmio_read32(mmio_addr);
			uint32_t header;

			if (id == 0xffffffff || id == 0) {
				continue;
			}

			pci_devices[cur_dev].busdevfn = mmio_addr;
			pci_devices[cur_dev].vendor_id = id&0xffff;
			pci_devices[cur_dev].device_id = id>>16U;

			cur_dev++;

			header = mmio_read32(mmio_addr + 0x0E);
			if (header & 0x80) {
				uintptr_t fn;
				for (fn = 0; fn<(8<<12); fn+=(1<<12)) {
					mmio_addr = address | bus | dev | fn;
					id = mmio_read32(mmio_addr);

					if (id == 0xffffffff || id == 0) {
						continue;
					}

					pci_devices[cur_dev].busdevfn = mmio_addr;
					pci_devices[cur_dev].vendor_id = id&0xffff;
					pci_devices[cur_dev].device_id = id>>16U;

					cur_dev++;
				}
			}
		}
	}

	if (num_dev != cur_dev) {
		return -1;
	}


	return 0;
}
