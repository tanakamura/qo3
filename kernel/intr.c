#include "kernel/intr.h"
#include <stdio.h>
#include "kernel/brk.h"
#include "kernel/intrinsics.h"

void
fatal(void)
{
	int c;
	puts("fatal!");
	dump_brk();
	while (1) {
		monitor(&c, 0, 0);
		mwait(3<<4|2, 0);
	}
}

void
cdiv_error(void)
{
	puts("divided by 0");
	fatal();
}

static void
dump_inst(uintptr_t eip, uintptr_t cs)
{
	int i;
	for (i=0; i<10; i++) {
		printf("%02x ", *(unsigned char*)(eip+i));
	}
	puts("");
}

void
cinvalid_opcode(int edi, int esi, int ebp, int esp,
		int ebx, int edx, int ecx, int eax,
		int eip, int cs, int eflags)
{
	dump_inst(eip, cs);
	printf("invalid opcode: %x, %x\n", eip, cs);
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

void
clapic_error(void)
{
	puts("lapic_error");
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
	fatal();
}
