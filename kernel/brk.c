#include "brk.h"

extern char _end[];
static uintptr_t brk_pos;

void *
sbrk_align_shift(intptr_t inc, unsigned int shift)
{
	uintptr_t mask = (((uintptr_t)1)<<shift)-1;
	uintptr_t add = mask-1;
	void *ret;

	brk_pos = (brk_pos + add) & mask;
	inc = (inc+add)&mask;

	ret = (void*)brk_pos;

	brk_pos += inc;

	return ret;
}

