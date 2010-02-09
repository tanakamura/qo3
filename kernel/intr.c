#include "kernel/intr.h"
#include <stdio.h>
#include "kernel/brk.h"
#include "kernel/intrinsics.h"
#include "kernel/bios.h"

void
fatal(void)
{
	int c;
	puts("fatal!");
	bios_system_reset();
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
	printf("cr0 = %08x\n", get_cr0());
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
GEN_UNHANDLED(cpage_fault);
GEN_UNHANDLED(cfp_error);
GEN_UNHANDLED(calignment_check);
GEN_UNHANDLED(cmachine_check);
GEN_UNHANDLED(csimd_float);

void
cstack_segment_fault(uintptr_t rip,
		     uintptr_t cs)
{
	printf("stack sagment fault: rip=%08x cs=%08x\n",
	       (int)rip, (int)cs);

	fatal();
}

void
cgeneral_protection(uintptr_t errcode,uintptr_t rip, uintptr_t cs, uintptr_t *saved_regs)
{
	printf("general protection %x @ %x\n", (int)errcode, (int)rip);
	fatal();
}
