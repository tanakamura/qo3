#ifndef QO3_INTR_H
#define QO3_INTR_H

#include "lapic.h"

void cdiv_error(void);
void cunknown_exception(int vec);
void clapic_timer(void);

void cnmi(void);
void coverflow(void);
void cbound(void);
void cbreakpoint(void);
void csegment_not_present(void);
void cstack_segment_fault(void);
void cpage_fault(void);
void cfp_error(void);
void calignment_check(void);
void cmachine_check(void);
void csimd_float(void);

void
cgeneral_protection(int edi, int esi, int ebp,int esp,int ebx,int edx,
		    int ecx,int eax,int errcode,int eip);


#define INTVEC_LAPIC_TIMER 64
#define INTVEC_LAPIC_ERROR 65

#define LAPIC_SET_LVT_TIMER(mode,mask) write_local_apic(LAPIC_LVT_TIMER, (mode)|(mask)|(INTVEC_LAPIC_TIMER))
#define LAPIC_SET_LVT_ERROR(mask) write_local_apic(LAPIC_LVT_ERROR, (mask)|(INTVEC_LAPIC_ERROR))
#endif
