#include "kernel/event.h"
#include <pmmintrin.h>
#include "intrinsics.h"
#include <stdio.h>

#define C_STATE(state,sub) ((state)<<4|(sub))

event_bits_t
wait_event(event_bits_t *ready_ptr, event_bits_t mask)
{
	while (1) {
		event_bits_t r = *ready_ptr;
		event_bits_t m = r & mask;
		if (m) {
			return r;
		}

		monitor(ready_ptr, 0, 0);
		lfence();
		r = *ready_ptr;
		m = r & mask;
		if (m) {
			return r;
		}

		mwait(C_STATE(3<<4,0), 0);
	}
}

