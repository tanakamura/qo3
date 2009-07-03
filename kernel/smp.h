#ifndef QO3_KERNEL_SMP_H
#define QO3_KERNEL_SMP_H
#include "memmap.h"

#define NUM_MAX_CPU 16

#define APBOOT_ADDR_4K 0x40
#define APBOOT_ADDR_SEGMENT (APBOOT_ADDR_4k*256)
#define APBOOT_ADDR (APBOOT_ADDR_4K*4096)
#define APBOOT_ADDR_FLAG (*(uint32_t volatile *)(APBOOT_ADDR_4K*4096 + 256 + 4))
#define APBOOT_ADDR_START (*(uint32_t volatile *)(APBOOT_ADDR_4K*4096 + 256))

#define PROCESSOR_ENABLED (1<<0)

struct smp_table_entry {
	int flags;
};

extern struct smp_table_entry smp_table[NUM_MAX_CPU];
extern unsigned char ap_stack[NUM_MAX_CPU][STACK_SIZE];
extern unsigned int have_too_many_cpus;
#endif
