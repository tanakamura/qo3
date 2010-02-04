#include "kernel/firstsegment-const.h"

static inline void *
get_segment16_addr(int code)
{
	unsigned short *linear_addr = (unsigned short*)(FIRSTSEGMENT_ADDR32);
	unsigned short off = linear_addr[code];

	return (void*)(((char*)linear_addr)+off);
}
