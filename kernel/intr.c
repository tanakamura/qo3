#include "kernel/intr.h"
#include <stdio.h>
#include "kernel/brk.h"
#include "kernel/intrinsics.h"
#include "kernel/bios.h"
#include "kernel/self-info.h"
#include "kernel/save-regs.h"

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
GEN_UNHANDLED(cfp_error);
GEN_UNHANDLED(calignment_check);
GEN_UNHANDLED(cmachine_check);
GEN_UNHANDLED(csimd_float);

static void
dump_regs(uintptr_t *saved_regs64)
{
	unsigned char *saved_regs = (unsigned char*)saved_regs64;

	printf("RAX = %llx\n", *(unsigned long long*)(saved_regs+SAVE_REG_OFF_RAX));
	printf("RBX = %llx\n", *(unsigned long long*)(saved_regs+SAVE_REG_OFF_RBX));
	printf("RCX = %llx\n", *(unsigned long long*)(saved_regs+SAVE_REG_OFF_RCX));
	printf("RDX = %llx\n", *(unsigned long long*)(saved_regs+SAVE_REG_OFF_RDX));
	printf("RSI = %llx\n", *(unsigned long long*)(saved_regs+SAVE_REG_OFF_RSI));
	printf("RDI = %llx\n", *(unsigned long long*)(saved_regs+SAVE_REG_OFF_RDI));
	printf("RSP = %llx\n", *(unsigned long long*)(saved_regs+SAVE_REG_OFF_RSP));
	printf("RBP = %llx\n", *(unsigned long long*)(saved_regs+SAVE_REG_OFF_RBP));
}

void
cstack_segment_fault(uintptr_t errcode,uintptr_t rip, uintptr_t cs, uintptr_t *saved_regs, uintptr_t flags)
{
	int off;
	const char *sym = addr2sym(&off, rip);

	printf("stack segment fault %lx @ %lx[%s+%x] flags=%lx\n",
	       (unsigned long)errcode,
	       (unsigned long)rip, sym, off,
	       (unsigned long)flags);
	dump_regs(saved_regs);

	fatal();
}

void
cgeneral_protection(uintptr_t errcode,uintptr_t rip, uintptr_t cs, uintptr_t *saved_regs)
{
	int off;
	const char *sym = addr2sym(&off, rip);

	printf("general protection %x @ %x[%s+%x]\n", (int)errcode, (int)rip, sym, off);
	dump_regs(saved_regs);


	fatal();
}

void
cpage_fault(uintptr_t errcode,uintptr_t rip, uintptr_t cs, uintptr_t *saved_regs)
{
	int off;
	uintptr_t fault_address;
	const char *sym = addr2sym(&off, rip);

	__asm__ ("mov %%cr2, %0":"=r"(fault_address));

	printf("page fault error=%x @ %x[%s+%x], addr=%lx\n", (int)errcode, (int)rip, sym, off, fault_address);
	dump_regs(saved_regs);


	fatal();
}
