#include "smp.h"

struct smp_table_entry smp_table[NUM_MAX_CPU];
unsigned char ap_stack[NUM_MAX_CPU][STACK_SIZE];

