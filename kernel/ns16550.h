#ifndef NS16550_H
#define NS16550_H

void ns16550_write(const char *buffer, int n);
void ns16550_read(char *buffer, int n);
void ns16550_init(void);
void ns16550_dump_registers(void);

#endif
