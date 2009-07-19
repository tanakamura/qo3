#ifndef QO3_KERNEL_BRK_H
#define QO3_KERNEL_BRK_H

#include <stdint.h>

void *sbrk_align_shift(intptr_t inc, unsigned int shift);

static inline void *
sbrk_align(intptr_t inc, unsigned int align)
{
	int pos = __builtin_ffs(align);
	return sbrk_align_shift(inc, 31-pos);
}

static inline
void *sbrk_size(intptr_t inc)
{
	return sbrk_align(inc, inc);
}

#define SBRK_TA(type, nelem) sbrk_align(sizeof(type)*(nelem), sizeof(type))
#define SBRK_T(type) sbrk_size(sizeof(type),sizeof(type))

#endif
