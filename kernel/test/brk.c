#define _end test_end
#include <stdio.h>

#include "../brk.h"
#include "../brk.c"

char test_end[1024*1024*128] __attribute__((aligned(8)));

int main()
{
	brk_pos = (uintptr_t)_end;
	printf("%x\n", brk_pos);
	printf("%p\n", sbrk_align8(3));
	printf("%p\n", sbrk_align8(3));
	printf("%p\n", sbrk_align8(3));
	printf("%p\n", sbrk_align4(8));
	printf("%p\n", sbrk_align4(3));
	printf("%p\n", sbrk_align4(3));
	printf("%p\n", sbrk_align4(4));
	printf("%p\n", sbrk_align4(4));

	return 0;
}
