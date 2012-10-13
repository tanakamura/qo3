#include <stdio.h>
#include <string.h>

#include "npr/bittree.h"
#include "npr/align.h"

#include "kernel/x8664-page.h"
#include "kernel/boot-flags.h"
#include "kernel/segment16.h"
#include "kernel/page.h"
#include "kernel/fatal.h"

extern uint32_t num_startup_2MB_page;

void
arch_init_kernel_address_space(void)
{
	
}

void
arch_init_mem_region(void)
{
	struct mem_node *node0 = &mem_nodes[0];
	short *e820_table_info;
	uint32_t *e820_table;
	int num_e820_entry, i;
	uint64_t j;
	uintptr_t max = 0;
	uintptr_t alloc_start = num_startup_2MB_page * 2048 * 1024;
	unsigned int alloc_tree_size, alloc_tree_page;
	int have_start_page = 0;
	int num_all_page, real_num_all_page = 0;
	void *tree_bits;
	struct npr_bittree *free_bits = &node0->regions[0].free_bits;

	e820_table_info = (short*)get_segment16_addr(E820_TABLE_INFO);
	if (e820_table_info[0] == 0) {
		puts("no smap");
		fatal();
	}

	num_e820_entry = e820_table_info[1];
	e820_table = (uint32_t*)get_segment16_addr(E820_TABLE);

	for (i=0; i<num_e820_entry; i++) {
		uint32_t *entry = &e820_table[5*i];

		if (entry[4] == 1) {
			uint64_t addr = ((uint64_t*)entry)[0];
			uint64_t len = ((uint64_t*)entry)[1];
			uint64_t end = addr + len;

			if ((alloc_start >= addr) && (alloc_start < end)) {
				have_start_page = 1;
			}

			if (end > max) {
				max = end;
			}
		}
	}

	if (! have_start_page) {
		printf("start page(%lx) is reserved!!\n", alloc_start);
		fatal();
	}

	max = NPR_ALIGN_DOWN(max, PAGESIZE_LEVEL0_SIZE);
	printf("memory max: %lx\n", max);

	tree_bits = (void*)alloc_start;

	num_all_page = max / PAGESIZE_LEVEL0_SIZE;
	alloc_tree_size = npr_bittree_byte_size(num_all_page);
	alloc_tree_page = NPR_CEIL_DIV(alloc_tree_size,PAGESIZE_LEVEL0_SIZE);

	printf("alloc %d page for page bits.\n", alloc_tree_page);

	npr_bittree_init(free_bits, num_all_page);

	/* clear all */
	memset(tree_bits, 0, alloc_tree_size);

	for (i=0; i<num_e820_entry; i++) {
		uint32_t *entry = &e820_table[5*i];

		if (entry[4] == 1) {
			uint64_t addr = ((uint64_t*)entry)[0];
			uint64_t len = ((uint64_t*)entry)[1];
			uint64_t end = addr + len;

			if ((addr & (PAGESIZE_LEVEL0_SIZE-1)) ||
			    (end & (PAGESIZE_LEVEL0_SIZE-1))) {
				printf("e820 region %llx-%llx is not aligned to page.\n", addr, end);
				fatal();
			}

			for (j=addr/PAGESIZE_LEVEL0_SIZE;
			     j<end/PAGESIZE_LEVEL0_SIZE;
			     j++) {
				npr_bittree_set(free_bits, tree_bits, j);
				real_num_all_page++;
			}
		}
	}

	/* reserve first page + tree bits */
	for (j=0; j<num_startup_2MB_page*512 + alloc_tree_page; j++) {
		if (npr_bittree_p(free_bits, tree_bits, j)) {
			npr_bittree_clear(free_bits, tree_bits, j);
			real_num_all_page--;
		}
	}

	node0->num_region = 1;
	node0->regions[0].begin = alloc_start;
	node0->regions[0].end = max;
	node0->regions[0].num_all_page = num_all_page;
	node0->regions[0].bittree_data = tree_bits;

	spinlock_init(&node0->regions[0].alloc_page_lock);

	printf("available pages: %d\n", num_all_page);
}
