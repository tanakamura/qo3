#ifndef QO3_KERNEL_EVENT_H
#define QO3_KERNEL_EVENT_H

#include <stdint.h>

typedef uint32_t event_bits_t;

#define all_event (~0)

event_bits_t wait_event_all(event_bits_t *ready_ptr, event_bits_t mask);
event_bits_t wait_event_any(event_bits_t *ready_ptr, event_bits_t mask);


#endif
