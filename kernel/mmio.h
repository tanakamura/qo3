#ifndef QO3_KERNEL_MMIO_H
#define QO3_KERNEL_MMIO_H

#define mmio_read32(p) (*((volatile unsigned int*)(p)))
#define mmio_write32(p,v) ((*((volatile unsigned int*)(p)))=(v))

#endif
