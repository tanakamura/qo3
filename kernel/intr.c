#include "intr.h"
#include <stdio.h>

void
fatal(void)
{
	while (1) {
		__asm__ __volatile__ ("hlt");
	}
	
}

void
cdiv_error(void)
{
	puts("divided by 0");
	fatal();
}

void
cinvalid_opcode(void)
{
	puts("invalid opcode");
	fatal();
}


void
cunknown_exception(void)
{
	puts("unknown exception");
	fatal();
}
