#include "kernel/hpet.h"
#include "kernel/mmio.h"
#include "kernel/qo3-acpi.h"
#include "kernel/int-assign.h"
#include "kernel/intrinsics.h"
#include "kernel/lapic.h"
#include "kernel/ioapic.h"
#include <stdio.h>

struct hpet_comparator {
	event_bits_t *ready_ptr;
	event_bits_t ready_bits;
};

struct hpet_comparator comparators[3];

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
	uint32_t cnf;
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

	/* enable replace legacy root */
	cnf = HPET_READ(HPET_GEN_CONF);
	cnf |= HPET_LEG_RT_CNF;
	HPET_WRITE(HPET_GEN_CONF, cnf);

	return HPET_SETUP_OK;
}

hpet_tick_t
hpet_usec_to_tick(unsigned int usec)
{
	return (usec * hpet_freq_khz) / 1000;
}

hpet_tick_t
hpet_msec_to_tick(unsigned int msec)
{
	return (msec * hpet_freq_khz);
}

void
chpet0_intr(void)
{
	struct hpet_comparator *c = &comparators[0];

	*(c->ready_ptr) |= c->ready_bits;
	sfence();

	hpet_stop();

	write_local_apic(LAPIC_EOI, 0);
}

void
hpet_oneshot(hpet_tick_t ticks,
	     int comparator,
	     event_bits_t *ready_ptr,
	     event_bits_t ready_bits)
{
	uint32_t cnf;
	if (comparator != 0) {
		printf("warn: support only comparator 0. (%d)\n", comparator);
		comparator = 0;
	}

	/* one shot, 32bit, interrupt enabled, irq=2, edge trigger */
	cnf = HPET_CONF_32MODE_CNF|HPET_CONF_INT_ENB_CNF;
	HPET_WRITE(HPET_TIMN_CONF(comparator), cnf);

	HPET_WRITE(HPET_MAIN_CNT, 0);
	HPET_WRITE(HPET_TIMN_COMPARATOR(comparator), ticks);

	/* mask clear, edge trigger, interrupt hi, destination physical,
	 * delivery fixed, vec=HPET0_VEC */
	ioapic_set_redirect32(HPET0_IRQ, /* fixme */
			      IOAPIC_DESTINATION_ID32(0),
			      IOAPIC_DELIVERY_FIXED|IOAPIC_VECTOR(HPET0_VEC));

	comparators[comparator].ready_ptr = ready_ptr;
	comparators[comparator].ready_bits = ready_bits;

	sfence();

	hpet_start();
}
