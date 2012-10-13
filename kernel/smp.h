#ifndef QO3_KERNEL_SMP_H
#define QO3_KERNEL_SMP_H
#include "memmap.h"
#include <stdint.h>

#define NUM_MAX_CPU 16

#define APBOOT_ADDR_4K 0x40
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

#define CACHE_LINE_WIDTH 64

extern unsigned char ap_command_mwait_line[NUM_MAX_CPU][CACHE_LINE_WIDTH] __attribute__((aligned(CACHE_LINE_WIDTH)));

void ap_thread(int apic_id);

typedef uint32_t ap_command_t;
void post_command_to_ap(int apic_id, ap_command_t command);

#define AP_COMMAND_HELLO 1
#define AP_COMMAND_RUN_BENCH 2

#endif
