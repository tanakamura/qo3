#include "brk.h"
#include <stdio.h>

extern char _end[];
static uintptr_t brk_pos;

void *
sbrk_align_shift(uintptr_t inc, unsigned int shift)
{
	uintptr_t bit = (((uintptr_t)1)<<shift);
	uintptr_t mask = ((uintptr_t)0)-bit;
	uintptr_t add = bit-1;
	void *ret;

	brk_pos = (brk_pos + add) & mask;
	inc = (inc+add)&mask;

	ret = (void*)brk_pos;

	brk_pos += inc;

	return ret;
}

void
brk_init()
{
	brk_pos = (uintptr_t)_end;
}

void
dump_brk(void)
{
	printf("brk used: %ld bytes\n",
	       brk_pos - (uintptr_t)_end);
}
