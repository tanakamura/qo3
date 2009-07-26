#include "wait.h"
#include "ich7.h"
#include "hpet.h"
#include "intrinsics.h"
#include <stdio.h>

static void
wait_tick(unsigned int ticks)
{
	unsigned int v;

	hpet_stop();
	HPET_WRITE(HPET_MAIN_CNT, 0);
	hpet_start();

	while (1) {
		v = HPET_READ(HPET_MAIN_CNT);
		if (v >= ticks) {
			break;
		}
	}

	hpet_stop();
}


void
wait_msec(unsigned int msec)
{
	wait_tick(msec * hpet_freq_khz);
}

void
wait_usec(unsigned int usec)
{
	wait_tick((usec * hpet_freq_khz)/1000);
}
