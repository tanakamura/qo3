#include "kernel/backtrace.h"
#include "kernel/self-info.h"
#include <stdio.h>

void
dump_backtrace(reg_t start_rbp, int depth, int indent)
{
	reg_t *b = (reg_t*)start_rbp;
	int i, j;
	for (i=0; i<depth; i++) {
		int off;
		reg_t prev_bp = b[0];
		reg_t prev_ip = b[1];
		const char *sym = addr2sym(&off, prev_ip);

		for (j=0; j<indent; j++) {
			putchar(' ');
		}

		printf("%p[%s + 0x%x]\n", (int*)prev_ip, sym, off);

		b = (reg_t*)prev_bp;
	}
}
