#include "acpi.h"
#include <stdio.h>
#include "brk.h"

#define SIG(a,b,c,d) (((d)<<24) | ((c)<<16) | ((b)<<8) | (a))
#define R8 ACPI_R8
#define R16 ACPI_R16
#define R32 ACPI_R32
#define R64 ACPI_R64

struct acpi_table acpi_table;

static int
table_checksum(uintptr_t addr, int n)
{
	int i;
	unsigned char sum = 0;
	unsigned char *ptr8 = (unsigned char*)addr;
	for (i=0; i<n; i++) {
		sum += ptr8[i];
	}
	if (sum == 0) {
		return 1;
	}
	return 0;
}

static int
is_valid_sig8_checksum(uintptr_t addr, uint32_t sig0, uint32_t sig1)
{
	uint32_t *ptr32 = (uint32_t *)addr;

	if ((ptr32[0] == sig0) && (ptr32[1] == sig1)) {
		return table_checksum(addr, 20);
	}

	return 0;
}

static int
is_valid_sig4(uintptr_t addr, uint32_t sig0)
{
	uint32_t *ptr32 = (uint32_t *)addr;

	return (ptr32[0] == sig0);
}



static uintptr_t
find_RSDP_region(uintptr_t begin,
		 uintptr_t end)
{
	uintptr_t cur;
	uint32_t sigRSD_ = SIG('R','S','D',' ');
	uint32_t sigPTR_ = SIG('P','T','R',' ');

	for (cur=begin; cur<end; cur+=16) {
		if (is_valid_sig8_checksum(cur, sigRSD_, sigPTR_)) {
			return cur;
		}
	}

	return 0;
}

uintptr_t
find_RSDP(void)
{
	uintptr_t ebda_addr;
	uintptr_t rsdp;

	ebda_addr = *(uint16_t*)0x40e;

	rsdp = find_RSDP_region(ebda_addr, ebda_addr+1024);
	if (rsdp) {
		/* printf("find RSDP @ 0x%x(EBDA)\n", (int)rsdp); */
		return rsdp;
	}

	/* 0xE0000 - 0xFFFFF */
	rsdp = find_RSDP_region(0xe0000, 0x100000);
	if (rsdp) {
		/* printf("find RSDP @ 0x%x\n", (int)rsdp); */
		return rsdp;
	}

	return 0;
}

static int
read_xsdt(struct build_acpi_table_error *err, uintptr_t xsdt)
{
	int r = is_valid_sig4(xsdt, SIG('X','S','D','T'));
	unsigned int num_entry, len;

	if (!r) {
		err->code = BUILD_ACPI_TABLE_UNKNOWN_SIG;
		err->u.addr = xsdt;
		return -1;
	}

	len = R32(xsdt,4);
	len -= 36;		/* 36 = offset to entry */
	num_entry = len / 8U;
	acpi_table.xsdt_nentry = num_entry;

	return 0;
}

static int
read_rsdt(struct build_acpi_table_error *err, uintptr_t rsdt)
{
	int r = is_valid_sig4(rsdt, SIG('R','S','D','T'));
	unsigned int num_entry, len;

	if (!r) {
		err->code = BUILD_ACPI_TABLE_UNKNOWN_SIG;
		err->u.addr = rsdt;
		return -1;
	}

	len = R32(rsdt,4);
	len -= 36;		/* 36 = offset to entry */
	num_entry = len / 4U;

	acpi_table.rsdt_nentry = num_entry;

	return 0;
}


static int
read_dsdt(struct build_acpi_table_error *err, uintptr_t dsdt)
{
	int len = R32(dsdt, 4);
	unsigned int lead_byte;
	unsigned int aml_len;
	unsigned int off = 36;

	len -= 36;		/* offset of block */

	while (len > 0) {
		unsigned int len_len;
		unsigned int blk_len;
		lead_byte = ACPI_R8(dsdt, off);
		len_len = lead_byte >> 6U;

		switch (len_len) {
		case 0:
			aml_len = lead_byte & 0x3f;
			break;
		case 1:
			aml_len = ((lead_byte & 0xf)<<8) + (R8(dsdt,off+1));
			break;

		case 2:
			aml_len = ((lead_byte & 0xf)<<16) + (R8(dsdt,off+1)<<8) + (R8(dsdt,off+2));
			break;
		case 3:
			aml_len = ((lead_byte & 0xf)<<24) + (R8(dsdt,off+1)<<16)
				+ (R8(dsdt,off+2)<<8) + + (R8(dsdt,off+3));
			break;
		}

		blk_len = len_len + aml_len;
		len -= blk_len;
		off += blk_len;
	}

	(void)err;
	return 0;
}


static int
read_facp(struct build_acpi_table_error *err, uintptr_t facp)
{
	uintptr_t dsdt;
	int r;

	dsdt = R32(facp, 40);
	r = is_valid_sig4(dsdt, SIG('D', 'S', 'D', 'T'));

	if (!r) {
		err->code = BUILD_ACPI_TABLE_UNKNOWN_SIG;
		err->u.addr = dsdt;
		return -1;
	}

	acpi_table.dsdt = dsdt;

	r = read_dsdt(err, dsdt);
	if (r < 0) {
		return -1;
	}

	return 0;
}


int
build_acpi_table(struct build_acpi_table_error *err)
{
	uintptr_t r;
	int result;

	r = find_RSDP();
	if (!r) {
		err->code = BUILD_ACPI_TABLE_RSDP_NOT_FOUND;
		return -1;
	}

	acpi_table.rsdp = r;
	acpi_table.rsdt = R32(r,16);
	acpi_table.xsdt = R64(r,24);

	if (acpi_table.xsdt) {
		result = read_xsdt(err, acpi_table.xsdt);
		if (result < 0) {
			return -1;
		}
	}

	result = read_rsdt(err, acpi_table.rsdt);
	if (result < 0) {
		return -1;
	}

	r = find_acpi_description_entry(SIG('F','A','C','P'));
	if (! r) {
		err->code = BUILD_ACPI_TABLE_FACP_NOT_FOUND;
		return -1;
	}
	acpi_table.facp = r;
	result = read_facp(err, acpi_table.facp);
	if (result < 0) {
		return -1;
	}

	return 0;
}

void
print_build_acpi_table_error(struct build_acpi_table_error *er)
{
	switch (er->code) {
	case BUILD_ACPI_TABLE_RSDP_NOT_FOUND:
		puts("rsdp not found");
		return;

	case BUILD_ACPI_TABLE_UNKNOWN_SIG:
		printf("unknown sig @ %08x\n", (int)er->u.addr);
		return;

	case BUILD_ACPI_TABLE_FACP_NOT_FOUND:
		puts("facp not found");
		return;
	}

	puts("unknown build acpi table error");
}

uintptr_t
find_acpi_description_entry(uint32_t sig)
{
	int i;
	int n = acpi_table.rsdt_nentry;
	uintptr_t rsdt = acpi_table.rsdt;
	uintptr_t xsdt = acpi_table.xsdt;

	if (rsdt) {
		for (i=0; i<n; i++) {
			uintptr_t addr = R32(rsdt, 36+i*4);
			if (sig == *(uint32_t*)addr) {
				return addr;
			}
		}
	}

	if (xsdt) {
		for (i=0; i<n; i++) {
			uintptr_t addr = R64(xsdt, 36+i*8);
			if (sig == *(uint32_t*)addr) {
				return addr;
			}
		}
	}

	return 0;
}
