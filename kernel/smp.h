#ifndef QO3_KERNEL_SMP_H
#define QO3_KERNEL_SMP_H

#define NUM_MAX_CPU 16

#define PROCESSOR_ENABLED (1<<0)
#define APBOOT_ADDR_4K 0xbd
#define APBOOT_ADDR_SEGMENT 0xbd00
#define APBOOT_ADDR (APBOOT_ADDR_4K*4096)
#define APBOOT_ADDR_FLAG (*(uint32_t*)(APBOOT_ADDR_4K*4096 + 256))

struct smp_table_entry {
	int flags;
};

extern struct smp_table_entry smp_table[NUM_MAX_CPU];

#endif
