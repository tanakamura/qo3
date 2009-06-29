#include "hpet.h"
#include "mmio.h"

void
hpet_start(uintptr_t hpet_base_addr)
{
	unsigned int cnf = mmio_read32(hpet_base_addr + HPET_GEN_CONF);
	cnf |= HPET_ENABLE_CNF;
	mmio_write32(hpet_base_addr + HPET_GEN_CONF, cnf);
}

void
hpet_stop(uintptr_t hpet_base_addr)
{
	unsigned int cnf = mmio_read32(hpet_base_addr + HPET_GEN_CONF);
	cnf &= ~HPET_ENABLE_CNF;
	mmio_write32(hpet_base_addr + HPET_GEN_CONF, cnf);
}

