#ifndef QO3_KERNEL_ACPI_H
#define QO3_KERNEL_ACPI_H

#include <stdint.h>

struct acpi_table {
	uintptr_t rsdp;
	uintptr_t rsdt;
	uintptr_t xsdt;
	uintptr_t facp;
	uintptr_t dsdt;
	int rsdt_nentry;
	int xsdt_nentry;
};

extern struct acpi_table acpi_table;
#define ACPI_RSDP() (acpi_table.rsdp)
#define ACPI_RSDT() (acpi_table.rsdt)
#define ACPI_XSDT() (acpi_table.xsdt)
#define ACPI_FACP() (acpi_table.facp)
#define ACPI_DSDT() (acpi_table.dsdt)

uintptr_t find_RSDP(void);

enum build_acpi_table_error_code {
	BUILD_ACPI_TABLE_RSDP_NOT_FOUND,
	BUILD_ACPI_TABLE_UNKNOWN_SIG,
	BUILD_ACPI_TABLE_FACP_NOT_FOUND
};

struct build_acpi_table_error {
	enum build_acpi_table_error_code code;
	union {
		uintptr_t addr;
	} u;
};

/* return negative if failed */
int build_acpi_table(struct build_acpi_table_error *err);

void print_build_acpi_table_error(struct build_acpi_table_error *err);

#define ACPI_SIG(a,b,c,d) (((d)<<24) | ((c)<<16) | ((b)<<8) | (a))
#define ACPI_R64ADDR(b,o) (*((uint32_t*)((b)+(o))))
#define ACPI_R32(b,o) (*((uint32_t*)((b)+(o))))
#define ACPI_R64(b,o) (*((uint64_t*)((b)+(o))))
#define ACPI_R16(b,o) (*((uint16_t*)((b)+(o))))
#define ACPI_R8(b,o) (*((uint8_t*)((b)+(o))))

/* return 0 if not found */
uintptr_t find_acpi_description_entry(uint32_t sig);


enum acpi_apic_type_code {
	ACPI_PROCESSOR_LOCAL_APIC = 0,
	ACPI_IO_APIC = 1,
	ACPI_INTERRUPT_SOURCE_OVERRIDE = 2,
	ACPI_NMI_SOURCE = 3,
	ACPI_LOCAL_APIC_NMI_STRUCTURE = 4,
	ACPI_LOCAL_APIC_ADDRESS_OVERRIDE_STRUCTURE = 5,
	ACPI_IO_SAPIC = 6,
	ACPI_LOCAL_SAPIC = 7,
	ACPI_PLATFORM_INTERRUPT_SOURCES = 8
};

void acpi_start(void);

#endif
