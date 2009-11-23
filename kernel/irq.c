#include "kernel/irq.h"
#include "kernel/bios.h"
#include "kernel/ioapic.h"
#include "kernel/lapic.h"
#include "kernel/int-assign.h"
#include <stdio.h>

static irq_handler_t pci_irq_handlers[PCI_IRQ_MAX][PCI_IRQ_DEV_MAX];
static void *irq_handler_data[PCI_IRQ_MAX][PCI_IRQ_DEV_MAX];
static int pci_irq_num_handler[PCI_IRQ_MAX];

void
pci_irq_init(void)
{
	int i;
	for (i=0; i<PCI_IRQ_MAX; i++) {
		pci_irq_num_handler[i] = 0;
	}
}

void
pci_irq_add(int irq, irq_handler_t handler, void *data)
{
	int n = pci_irq_num_handler[irq];

	/* assumes PCI irq is
	 * IRQ_BEGIN(16) <= irq < IRQ_DEV_MAX(24) */

	if (irq < PCI_IRQ_BEGIN) {
		printf("invalid IRQ @ IRQ%d\n", irq);
		bios_system_reset();
	}

	if (n >= PCI_IRQ_DEV_MAX) {
		printf("too many devices @ IRQ%d\n", irq);
		bios_system_reset();
	}

	pci_irq_handlers[irq][n] = handler;
	irq_handler_data[irq][n] = data;

	if (n == 0) {
		ioapic_set_redirect32(irq,
				      IOAPIC_DESTINATION_ID32(0),
				      IOAPIC_LEVEL_TRIGGER|
				      IOAPIC_INTERRUPT_PIN_LO|
				      IOAPIC_DELIVERY_FIXED|IOAPIC_VECTOR(PCI_IRQ16_VEC + (irq-16)));
	}

	pci_irq_num_handler[irq] = n+1;
}

void
pci_irq(int irq)
{
	int i, n;
	n = pci_irq_num_handler[irq];

	for (i=0; i<n; i++) {
		(*pci_irq_handlers[irq][i])(irq, irq_handler_data[irq][i]);
	}

	write_local_apic(LAPIC_EOI, 0);
}
