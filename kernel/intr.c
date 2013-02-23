#include "kernel/intr.h"
#include <stdio.h>
#include "kernel/brk.h"
#include "kernel/intrinsics.h"
#include "kernel/bios.h"
#include "kernel/self-info.h"
#include "kernel/save-regs.h"
#include "kernel/backtrace.h"

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
GEN_UNHANDLED(csegment_not_present);
GEN_UNHANDLED(cfp_error);
GEN_UNHANDLED(calignment_check);
GEN_UNHANDLED(cmachine_check);
GEN_UNHANDLED(csimd_float);

#define REF_REG(sr,off) (*(uintptr_t*)(((unsigned char*)sr)+off))

void
cbreakpoint(void)
{
	puts("breakpoint");
}

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

#define INTR_FRAME_ERROR_CODE 0
#define INTR_FRAME_RIP 1
#define INTR_FRAME_CS 2
#define INTR_FRAME_RF 3
#define INTR_FRAME_RSP 4
#define INTR_FRAME_SS 5

static void
dump_intr_frame(uintptr_t *intr_frame)
{
	printf("SS = %lx\n", intr_frame[INTR_FRAME_SS]);
	printf("CS = %lx\n", intr_frame[INTR_FRAME_CS]);
}


void
cstack_segment_fault(uintptr_t *intr_frame, uintptr_t *saved_regs)
{
	int off;
	reg_t rip = intr_frame[INTR_FRAME_RIP];
	const char *sym = addr2sym(&off, rip);
	uintptr_t errcode = intr_frame[INTR_FRAME_ERROR_CODE];
	uintptr_t flags = intr_frame[INTR_FRAME_RF];

	printf("stack segment fault %lx @ 0x%lx[%s + 0x%x] flags=%lx\n",
	       errcode,
	       rip, sym, off,
	       flags);
	dump_intr_frame(intr_frame);
	dump_regs(saved_regs);

	fatal();
}

void
cgeneral_protection(uintptr_t *intr_frame, uintptr_t *saved_regs)
{
	int off;
	reg_t rip = intr_frame[INTR_FRAME_RIP];
	const char *sym = addr2sym(&off, rip);
	uintptr_t errcode = intr_frame[INTR_FRAME_ERROR_CODE];

	printf("general protection %x @ 0x%x[%s + 0x%x]\n", (int)errcode, (int)rip, sym, off);
	dump_intr_frame(intr_frame);
	dump_regs(saved_regs);
	dump_backtrace(REF_REG(saved_regs,SAVE_REG_OFF_RBP), 4, 4);

	fatal();
}

#define ENTRY_ADDR(e) (((e)&((1ULL<<52)-1)) - ((1<<12)-1))
#define ENTRY_ACCESS(e) (((e)>>5)&1)

static void
dump_page_entry(const char *tag, uint64_t *entry_addr)
{
	uint64_t entry = (*entry_addr);
	uintptr_t addr = ENTRY_ADDR(entry);
	int avl = (entry>>9)&0x7;
	int mbz = (entry>>7)&0x3;
	int a = ENTRY_ACCESS(entry);
	int pcd = (entry>>4)&1;
	int pwt = (entry>>3)&1;
	int us = (entry>>2)&1;
	int rw = (entry>>1)&1;
	int p = (entry>>0)&1;

	printf("%s(%p): addr=%p, avl=%d, mbz=%d, a=%d, pcd=%d, pwt=%d, us=%d, rw=%d, p=%d\n",
	       tag,
	       entry_addr,
	       (void*)addr, avl, mbz, a, pcd, pwt, us, rw, p);
}

void
cpage_fault(uintptr_t *intr_frame, uintptr_t *saved_regs)
{
	int off;
	uintptr_t fault_address, cr3;
	reg_t rip = intr_frame[INTR_FRAME_RIP];
	const char *sym = addr2sym(&off, rip);
	uintptr_t errcode = intr_frame[INTR_FRAME_ERROR_CODE];
	int pml4_off, pdp_off, pd_off, pt_off, page_off;

	__asm__ ("mov %%cr2, %0":"=r"(fault_address));
	__asm__ ("mov %%cr3, %0":"=r"(cr3));

	printf("page fault error=%x @ 0x%x[%s + 0x%x], addr=%lx\n", (int)errcode, (int)rip, sym, off, fault_address);

	pml4_off = (fault_address>>39)&((1<<10)-1);
	pdp_off = (fault_address>>30)&((1<<10)-1);
	pd_off = (fault_address>>21)&((1<<10)-1);
	pt_off = (fault_address>>12)&((1<<10)-1);
	page_off = fault_address & ((1<<12)-1);

	printf("pmlbase=%p, pml4=%x, pdp=%x, pd=%x, pt=%x, page_off=%x\n",
	       (void*)cr3, pml4_off, pdp_off, pd_off, pt_off, page_off);

	/* trace page table */
	{
		uint64_t *pml4e_addr, *pdpe_addr, *pde_addr, *pte_addr, *page_addr;
		pml4e_addr = (uint64_t*)(cr3+pml4_off);
		dump_page_entry("pml4e", pml4e_addr);
		if (! ENTRY_ACCESS(*pml4e_addr)) {
			puts("pml4 fault");
			goto fault;
		}
		pdpe_addr = (uint64_t*)(ENTRY_ADDR((*pml4e_addr)) + pdp_off);
		dump_page_entry("pdpe", pdpe_addr);
		if (! ENTRY_ACCESS(*pdpe_addr)) {
			puts("pdp fault");
			goto fault;
		}
		pde_addr = (uint64_t*)(ENTRY_ADDR((*pdpe_addr)) + pd_off);
		dump_page_entry("pde", pde_addr);
		if (! ENTRY_ACCESS(*pde_addr)) {
			puts("pd fault");
			goto fault;
		}
		pte_addr = (uint64_t*)(ENTRY_ADDR((*pde_addr)) + pt_off);
		dump_page_entry("pte", pte_addr);
		if (! ENTRY_ACCESS(*pte_addr)) {
			puts("pt fault");
			goto fault;
		}
		page_addr = (uint64_t*)(ENTRY_ADDR(*pte_addr) + page_off);
		printf("physical = %p\n", page_addr);

	fault:
		;
	}

	dump_intr_frame(intr_frame);
	dump_regs(saved_regs);
	dump_backtrace(REF_REG(saved_regs,SAVE_REG_OFF_RBP), 4, 4);

	fatal();
}
