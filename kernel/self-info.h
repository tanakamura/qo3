#ifndef QO3_KERNEL_SELF_INFO_H
#define QO3_KERNEL_SELF_INFO_H

#include <stdint.h>

extern unsigned char addr_symbol_table[];

unsigned int addr2entry(int *off_out, uintptr_t addr);
const char *entry2symbol(unsigned int entry);

const char *addr2sym(int *off_out, uintptr_t addr);

#endif
