#ifndef QO3_KERNEL_INT_ASSIGN_H
#define QO3_KERNEL_INT_ASSIGN_H

/* int vectors (you should fix boot.s) */
#define LAPIC_TIMER_VEC 64
#define LAPIC_ERROR_VEC 65
#define HPET0_VEC 66
#define COM0_VEC 67
#define ACPI_VEC 68

/* IRQ */
#define HPET0_IRQ 2		/* legacy replace */
#define COM0_IRQ 4		/* serial */

#endif
