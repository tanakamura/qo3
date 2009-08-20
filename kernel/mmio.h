#ifndef QO3_KERNEL_MMIO_H
#define QO3_KERNEL_MMIO_H

#include <stdint.h>

#define mmio_read32(p) (*((volatile uint32_t*)(p)))
#define mmio_write32(p,v) ((*((volatile uint32_t*)(p)))=(v))
#define mmio_read16(p) (*((volatile uint16_t*)(p)))
#define mmio_write16(p,v) ((*((volatile uint16_t*)(p)))=(v))
#define mmio_read8(p) (*((volatile uint8_t*)(p)))
#define mmio_write8(p,v) ((*((volatile uint8_t*)(p)))=(v))

#endif
