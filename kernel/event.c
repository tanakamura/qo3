#include "kernel/event.h"
#include "intrinsics.h"
#include <stdio.h>

#define C_STATE(state,sub) ((state)<<4|(sub))

static inline event_bits_t
wait_event(event_bits_t *ready_ptr, event_bits_t mask, int is_any)
{
	while (1) {
		event_bits_t r = *ready_ptr;
		event_bits_t m = r & mask;
		if (is_any) {
			if (m) {
				return r;
			}
		} else {
			if (m == mask) {
				return r;
			}
		}

		monitor(ready_ptr, 0, 0);
		lfence();
		r = *ready_ptr;
		m = r & mask;
		if (is_any) {
			if (m) {
				return r;
			}
		} else {
			if (m == mask) {
				return r;
			}
		}

		mwait(C_STATE(3<<4,0), 0);
	}
}


event_bits_t
wait_event_any(event_bits_t *ready_ptr, event_bits_t mask)
{
	return wait_event(ready_ptr, mask, 1);
}

event_bits_t
wait_event_all(event_bits_t *ready_ptr, event_bits_t mask)
{
	return wait_event(ready_ptr, mask, 0);
}
