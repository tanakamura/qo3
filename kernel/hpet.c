#include "hpet.h"
#include "mmio.h"
#include "qo3-acpi.h"
#include <stdio.h>

uintptr_t hpet_base_addr;
int hpet_flags;
unsigned int hpet_freq_khz;

void
hpet_start(void)
{
	uint32_t cnf = mmio_read32(hpet_base_addr + HPET_GEN_CONF);
	cnf |= HPET_ENABLE_CNF;
	mmio_write32(hpet_base_addr + HPET_GEN_CONF, cnf);
}

void
hpet_stop(void)
{
	uint32_t cnf = mmio_read32(hpet_base_addr + HPET_GEN_CONF);
	cnf &= ~HPET_ENABLE_CNF;
	mmio_write32(hpet_base_addr + HPET_GEN_CONF, cnf);
}

static void
hpet_setup_hz(void)
{
	unsigned int hpet_period;
	hpet_period = mmio_read32(hpet_base_addr + HPET_GCAP_HI);
	hpet_freq_khz = 1000000000/(hpet_period/1000);
}

enum hpet_setup_error_code
hpet_setup(void)
{
	uintptr_t addr = find_acpi_description_entry(ACPI_SIG('H', 'P', 'E', 'T'));
	int num;
	int pa;
	if (addr == 0) {
		hpet_base_addr = 0xfed00000;
		hpet_setup_hz();
		return HPET_SETUP_NOT_FOUND;
	} else {
		num = ACPI_R8(addr, 52);
		if (num != 0) {
			hpet_base_addr = 0xfed00000;
			hpet_setup_hz();
			return HPET_SETUP_TOO_MANY_TIMERS;
		}

		hpet_base_addr = ACPI_R32(addr, 40 + 4);

		hpet_flags = 0;
		pa = ACPI_R8(addr, 8);
		if (pa == 0) {
			/* no guarantee page protection */
		} else if (pa == 1) {
			/* 4k protect */
			hpet_flags |= HPET_FLAG_4K;
		} else if (pa == 2) {
			/* 64k protect */
			hpet_flags |= HPET_FLAG_64K;
		}
		hpet_setup_hz();
	}

	return HPET_SETUP_OK;
}
