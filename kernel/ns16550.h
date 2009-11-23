#ifndef NS16550_H
#define NS16550_H

#include "kernel/event.h"

void ns16550_write_poll(const char *buffer, int n);
void ns16550_write_text_poll(const char *buffer, int n);

void ns16550_read_poll(char *buffer, int n);
void ns16550_init(void);

/* call after ioapic */
void ns16550_init_intr(void);

void ns16550_dump_registers(void);

void ns16550_read(void *buffer, int n, event_bits_t *ready_ptr, event_bits_t ready_bits);

#endif
