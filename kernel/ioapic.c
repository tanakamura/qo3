#include "ioapic.h"
#include "lapic.h"		/* depends apic_info */
#include "mmio.h"
#include <stdio.h>


void ioapic_set_redirect32(int irq,
			   uint32_t entry_val_hi,
			   uint32_t entry_val_lo)
{
	uintptr_t ioapic_addr = apic_info.ioapic_addr;
	uintptr_t ind = ioapic_addr;
	uintptr_t data = ioapic_addr + 0x10;
	uint32_t entry = (irq*2) + 0x10;

	mmio_write32(ind, entry);
	mmio_write32(data, entry_val_lo);
	mmio_write32(ind, entry+1);
	mmio_write32(data, entry_val_hi);
}
