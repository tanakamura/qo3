#ifndef QO3_KERNEL_HPET_H
#define QO3_KERNEL_HPET_H

#include <stdint.h>

#define HPET_GCAP	0x0000
#define HPET_GCAP_HI	0x0004
#define HPET_GEN_CONF	0x0010

#define HPET_ENABLE_CNF (1<<0)
#define HPET_LEG_RT_CNF	(1<<1)

#define HPET_GINTR_STA	0x0020
#define HPET_MAIN_CNT	0x00f0
#define HPET_MAIN_CNT_HI	0x00f4

#define HPET_TIMN_CONF(N) (0x00100+0x0020*(N) + 0x0000)
#define HPET_TIMN_COMP(N) (0x00100+0x0020*(N) + 0x0008)
#define HPET_TIMN_FSBIR(N) (0x00100+0x0020*(N) + 0x0010)

#define HPET_TIM0_CONF HPET_TIMN_CONF(0)
#define HPET_TIM0_COMP HPET_TIMN_COMP(0)
#define HPET_TIM0_FSBIR HPET_TIMN_FSBIR(0)
#define HPET_TIM1_CONF HPET_TIMN_CONF(1)
#define HPET_TIM1_COMP HPET_TIMN_COMP(1)
#define HPET_TIM1_FSBIR HPET_TIMN_FSBIR(1)
#define HPET_TIM2_CONF HPET_TIMN_CONF(2)
#define HPET_TIM2_COMP HPET_TIMN_COMP(2)
#define HPET_TIM2_FSBIR HPET_TIMN_FSBIR(2)

void hpet_stop(uintptr_t hpet_base_addr);
void hpet_start(uintptr_t hpet_base_addr);

#endif
