#include "kernel/lapic.h"
#include "kernel/qo3-acpi.h"
#include "kernel/smp.h"

struct apic_info apic_info;

enum apic_setup_error_code
apic_setup()
{
	uintptr_t apic = find_acpi_description_entry(ACPI_SIG('A','P','I','C'));
	int off, len;
	int num_processor;
	uintptr_t ioapic_addr = 0;
	
	if (!apic) {
		return APIC_SETUP_ENTRY_NOT_FOUND;
	}
	len = ACPI_R32(apic, 4);

	len -= 44;		/* offset to APIC structure */
	off = 44;

	num_processor = 0;

	while (len > 0) {
		enum acpi_apic_type_code code = ACPI_R8(apic, off);
		int str_len = ACPI_R8(apic, off+1);
		int id;
		
		switch (code) {
		case ACPI_PROCESSOR_LOCAL_APIC:
			id = ACPI_R8(apic, off + 3);
			if (id >= NUM_MAX_CPU) {
				return APIC_SETUP_TOO_MANY_PROCESSORS;
			}
			num_processor++;
			break;

		case ACPI_IO_APIC:
			if (ioapic_addr != 0) {
				return APIC_SETUP_TOO_MANY_IOAPIC;
			}
			ioapic_addr = ACPI_R32(apic, off+4);
			apic_info.ioapic_id = ACPI_R8(apic, off+2);
			break;

		default:
			break;
		}

		off += str_len;
		len -= str_len;
	}

	if (ioapic_addr == 0) {
		return APIC_SETUP_IOAPIC_NOT_FOUND;
	}

	apic_info.ioapic_addr = ioapic_addr;
	apic_info.num_processor = num_processor;

	return APIC_SETUP_OK;
}
