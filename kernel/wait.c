#include "wait.h"
#include "ich7.h"
#include "hpet.h"
#include "intrinsics.h"
#include <stdio.h>

static unsigned int hpet_freq_khz;

void
wait_setup(void)
{
	unsigned int hpet_period;
	hpet_period = ICH7_HPET_READ(HPET_GCAP_HI);
	hpet_freq_khz = 1000000000 / (hpet_period / 1000);
}

void
wait_msec(unsigned int msec)
{
	unsigned int ticks;

	hpet_stop(ICH7_HPET_ADDR_BASE);
	ICH7_HPET_WRITE(HPET_MAIN_CNT, 0);
	ticks = msec * hpet_freq_khz;
	hpet_start(ICH7_HPET_ADDR_BASE);

	while (1) {
		unsigned int v;
		v = ICH7_HPET_READ(HPET_MAIN_CNT);
		if (v >= ticks) {
			break;
		}
	}

	hpet_stop(ICH7_HPET_ADDR_BASE);
}
