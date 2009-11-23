#ifndef QO3_KERNEL_IRQ_H
#define QO3_KERNEL_IRQ_H

enum irq_handle_status {
	IRQ_HANDLED,
	IRQ_IGNORE
};

typedef enum irq_handle_status (*irq_handler_t)(int irq, void *data);

/* These values depend PC. */
#define PCI_IRQ_BEGIN 16
#define PCI_IRQ_MAX 24		/* Server may have more IRQ.. */
#define PCI_IRQ_DEV_MAX 8

void pci_irq_add(int irq, irq_handler_t handler, void *data);
void pci_irq_del(int irq, irq_handler_t handler);

void pci_irq_init(void);

#endif
