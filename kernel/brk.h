#ifndef QO3_KERNEL_BRK_H
#define QO3_KERNEL_BRK_H

#include <stdint.h>

void *sbrk_align_shift(uintptr_t inc, unsigned int shift);

static inline void *
sbrk_align(uintptr_t inc, unsigned int align)
{
	int pos = __builtin_ffs(align);
	return sbrk_align_shift(inc, 31-pos);
}

static inline void*
sbrk_align4(uintptr_t inc)
{
	return sbrk_align_shift(inc, 2);
}
static inline void *
sbrk_align8(uintptr_t inc)
{
	return sbrk_align_shift(inc, 3);
}


static inline
void *sbrk_size(uintptr_t inc)
{
	return sbrk_align(inc, inc);
}

#define SBRK_TA(type, nelem) sbrk_align(sizeof(type)*(nelem), sizeof(type))
#define SBRK_TA4(type, nelem) sbrk_align4(sizeof(type)*(nelem))
#define SBRK_TA8(type, nelem) sbrk_align8(sizeof(type)*(nelem))

#define SBRK_T(type) sbrk_size(sizeof(type),sizeof(type))

void brk_init(void);

#endif
