#include "kernel/smp.h"
#include "kernel/intrinsics.h"
#include "kernel/bench.h"
#include <stdio.h>

struct smp_table_entry smp_table[NUM_MAX_CPU];
unsigned char ap_stack[NUM_MAX_CPU][STACK_SIZE];
unsigned char ap_command_mwait_line[NUM_MAX_CPU][CACHE_LINE_WIDTH] __attribute__((aligned(CACHE_LINE_WIDTH)));

static void
run_command(int apic_id, ap_command_t command)
{
	switch (command) {
	case AP_COMMAND_RUN_BENCH:
		run_bench_ap(apic_id);
		break;

	case AP_COMMAND_HELLO:
		printf("hello@%d", apic_id);
		break;

	default:
		puts("unknown ap command");
		break;
	}
}

void
ap_thread(int apic_id)
{
	ap_command_t *command_ptr = (ap_command_t*)ap_command_mwait_line[apic_id];
	ap_command_t command;

	while (1) {
		command = *command_ptr;
		if (command) {
			*command_ptr = 0;
			run_command(apic_id, command);
			continue;
		}

		monitor(command_ptr, 0, 0);

		lfence();
		command = *command_ptr;
		if (command) {
			*command_ptr = 0;
			run_command(apic_id, command);
			continue;
		}

		mwait(3<<4|2, 0);
	}
}

void
post_command_to_ap(int apic_id, ap_command_t command)
{
	*((ap_command_t*)ap_command_mwait_line[apic_id]) = command;
}
