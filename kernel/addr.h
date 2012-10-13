#ifndef QO3_KERNEL_ADDR_H
#define QO3_KERNEL_ADDR_H

#ifdef __x86_64__

#define ADDR_LOW32(ptr) (((uintptr_t)(ptr))&0xffffffff)
#define ADDR_HI32(ptr) ((((uintptr_t)(ptr))>>32)&0xffffffff)

#else
#define ADDR_LOW32(ptr) (((uintptr_t)(ptr)))
#define ADDR_HI32(ptr) (0)

#endif

#endif
