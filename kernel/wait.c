#include "wait.h"
#include "ich7.h"
#include "hpet.h"
#include "intrinsics.h"
#include <stdio.h>

void
wait_msec(unsigned int msec)
{
	event_bits_t ready;
	hpet_oneshot(hpet_msec_to_tick(msec), 0, &ready, 1);
	wait_event(&ready, 1);
}

void
wait_usec(unsigned int usec)
{
	event_bits_t ready;
	hpet_oneshot(hpet_msec_to_tick(usec), 0, &ready, 1);
	wait_event(&ready, 1);
}
