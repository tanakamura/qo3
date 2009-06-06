#ifndef QO3_KERNEL_MMIO_H
#define QO3_KERNEL_MMIO_H

#define mmio_read32(p) (*((unsigned int*)(p)))
#define mmio_write32(p,v) ((*((unsigned int*)(p)))=(v))

#endif
