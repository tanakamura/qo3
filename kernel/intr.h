#ifndef QO3_INTR_H
#define QO3_INTR_H

#include "kernel/lapic.h"

struct intr_regs {
	uint64_t rax;
	uint64_t rbx;
	uint64_t rcx;
	uint64_t rdx;
	uint64_t rsi;
	uint64_t rdi;
	uint64_t rbp;
	uint64_t rsp_pad;		// dont use

	uint64_t r8;
	uint64_t r9;
	uint64_t r11;
	uint64_t r12;
	uint64_t r13;
	uint64_t r14;
	uint64_t r15;
	uint64_t r16;

	/* 128byte */

	char xmm0[16][16];	// 256byte

	/* 384 byte */

	uint64_t rip;
	uint64_t cs;
	uint64_t rflags;
	uint64_t rsp;
	uint64_t ss;
};

void cdiv_error(void);
void cunknown_exception(int vec);
void clapic_timer(void);

void cnmi(void);
void coverflow(void);
void cbound(void);
void csegment_not_present(void);
void cfp_error(void);
void calignment_check(void);
void cmachine_check(void);
void csimd_float(void);

#define INTVEC_LAPIC_TIMER 64
#define INTVEC_LAPIC_ERROR 65

#define LAPIC_SET_LVT_TIMER(mode,mask) write_local_apic(LAPIC_LVT_TIMER, (mode)|(mask)|(INTVEC_LAPIC_TIMER))
#define LAPIC_SET_LVT_ERROR(mask) write_local_apic(LAPIC_LVT_ERROR, (mask)|(INTVEC_LAPIC_ERROR))
#endif
