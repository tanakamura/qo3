#ifndef QO3_KERNEL_IOAPIC_H
#define QO3_KERNEL_IOAPIC_H

#include <stdint.h>

#define IOAPIC_DESTINATION_ID(n) (((uint64_t)(n))<<56ULL)
#define IOAPIC_EDID(n) (((uint64_t)(n))<<48ULL) /* ?? */

#define IOAPIC_DESTINATION_ID32(n) (((uint32_t)(n))<<(56-32)) /* hi */
#define IOAPIC_EDID32(n) (((uint32_t)(n))<<(48-32)) /* hi */

#define IOAPIC_MASK (1<<16)
#define IOAPIC_LEVEL_TRIGGER (1<<15)
#define IOAPIC_REMOTE_IRR (1<<14)
#define IOAPIC_INTERRUPT_PIN_LO (1<<13)
#define IOAPIC_DELIVERY_PENDING (1<<12) /* ro */
#define IOAPIC_DESITINATION_LOGICAL (1<<11)
#define IOAPIC_DELIVERY_FIXED ((0x00)<<8) /* 000 */
#define IOAPIC_DELIVERY_LOWEST_PRIORITY ((0x01)<<8) /* 001 */
#define IOAPIC_DELIVERY_EXTINT ((0x07)<<8) /* 111 */
#define IOAPIC_VECTOR(n) (n)

void ioapic_set_redirect64(int irq,
			   uint64_t entry_val);

void ioapic_set_redirect32(int irq,
			   uint32_t entry_val_hi,
			   uint32_t entry_val_lo);

#endif
