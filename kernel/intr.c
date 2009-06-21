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
cunknown_exception(int vec)
{
	printf("unknown exception __%d__\n", vec);
	fatal();
}

void
clapic_timer(void)
{
	puts("lapic_timer");
	write_local_apic(LAPIC_EOI, 0);
}

#define GEN_UNHANDLED(name)			\
void name(void) {				\
	puts(#name);				\
	fatal();				\
}

GEN_UNHANDLED(cnmi);
GEN_UNHANDLED(coverflow);
GEN_UNHANDLED(cbound);
GEN_UNHANDLED(cbreakpoint);
GEN_UNHANDLED(csegment_not_present);
GEN_UNHANDLED(cstack_segment_fault);
GEN_UNHANDLED(cpage_fault);
GEN_UNHANDLED(cfp_error);
GEN_UNHANDLED(calignment_check);
GEN_UNHANDLED(cmachine_check);
GEN_UNHANDLED(csimd_float);

void
cgeneral_protection(int edi, int esi, int ebp, int esp,
		    int ebx, int edx, int ecx, int eax,
		    int errcode, int eip)
{
	(void)edi;
	(void)esi;
	(void)ebp;
	(void)esp;
	(void)ebx;
	(void)edx;
	(void)ecx;
	(void)eax;
	(void)eip;

	printf("general protection %x\n", errcode);
}
