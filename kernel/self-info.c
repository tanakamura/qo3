#include "kernel/self-info.h"
#include <stdint.h>
#include <stdio.h>

unsigned int
addr2entry(int *off_out, uintptr_t addr)
{
	uint32_t *tbl = (uint32_t*)addr_symbol_table;
	unsigned int num_entry = tbl[0];
	uint32_t *addrs = tbl+2;

	unsigned int i;

	for (i=0; i<num_entry; i++) {
		if (addrs[i] > addr) {
			break;
		}
	}

	if (i == 0) {
		*off_out = addr - addrs[i];
		return 0;
	}

	*off_out = addr - addrs[i-1];

	return i-1;
}

const char *
entry2symbol(unsigned int entry)
{
	uint32_t *tbl = (uint32_t*)addr_symbol_table;
	unsigned int sym_off;

	unsigned int num_entry = tbl[0];
	unsigned int off_to_syms = tbl[1];
	uint32_t *entry2sym = tbl + 2 + num_entry;

	sym_off = entry2sym[entry];

	return (const char*)(addr_symbol_table + off_to_syms + sym_off);
}

const char *
addr2sym(int *off_out, uintptr_t addr)
{
	unsigned int e = addr2entry(off_out, addr);
	return entry2symbol(e);
}
