#ifndef QO3_KERNEL_INT_ASSIGN_H
#define QO3_KERNEL_INT_ASSIGN_H

/* int vectors (you should fix boot.s) */
#define LAPIC_TIMER_VEC 64
#define LAPIC_ERROR_VEC 65
#define HPET0_VEC 66
#define COM0_VEC 67
#define ACPI_VEC 68

/* Assumes PCI irqs are 16-23 */
#define PCI_IRQ16_VEC 69
#define PCI_IRQ17_VEC (PCI_IRQ16+1)
#define PCI_IRQ18_VEC (PCI_IRQ16+2)
#define PCI_IRQ19_VEC (PCI_IRQ16+3)
#define PCI_IRQ20_VEC (PCI_IRQ16+4)
#define PCI_IRQ21_VEC (PCI_IRQ16+5)
#define PCI_IRQ22_VEC (PCI_IRQ16+6)
#define PCI_IRQ23_VEC (PCI_IRQ16+7)

/* IRQ */
#define HPET0_IRQ 2		/* legacy replace */
#define COM0_IRQ 4		/* serial */


#endif
