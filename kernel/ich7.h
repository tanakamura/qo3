#ifndef QO3_KERNEL_ICH7_H
#define QO3_KERNEL_ICH7_H

#include "mmio.h"

#define IOAPIC_INDEX 0xfec00000
#define IOAPIC_DATA 0xfec00010
#define IOAPIC_EOI 0xfec00040

#define IOAPIC_ID 0x00
#define IOAPIC_VER 0x01
#define IOAPIC_REDIR_TBL0L 0x10
#define IOAPIC_REDIR_TBL0H 0x11

#define ich7_read mmio_read32
#define ich7_write mmio_write32

#endif
