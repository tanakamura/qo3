#ifndef QO3_KERNEL_PAGE_H
#define QO3_KERNEL_PAGE_H

#include <stdint.h>
#include "kernel/x8664-page.h"
#include "npr/bitmap.h"

/* architecture independent page structure
 *
 *  address_space -> page_table -> pte -> physical addr
 *
 *  address_space_t : single virtual environment
 *  page_table_t    : page table
 *  pte             : pte
 *  
 *  - address_space_t init_address_space(uintptr_t mem)
 *   create empty address space @ mem
 *
 *  - address_space_t init_page_table(uintptr_t mem)
 *   create empty page table space @ mem
 *
 *  - void start_alloc_mm_entry_from_address_space(address_space_t as, al)
 *  - void start_alloc_mm_entry_from_page_table(page_table_t as, al)
 *
 *  - enum alloc_mm_entry_result alloc_pte(mm_entry_allocator *al);
 *
 *  - void push_mem(mm_entry_allocator *al, phys_ptr_t mem);
 */


enum alloc_mm_entry_result {
	ALLOC_MM_ENTRY_CLAIM_MEM,
	ALLOC_MM_ENTRY_DONE,
	ALLOC_MM_ENTRY_FAILED_FULL
};

struct mm_entry_allocator {
	struct arch_mm_entry_allocator arch;
	int request_mem_size;
	int request_mem_align_shift;
};

static inline void
start_alloc_mm_entry_from_address_space(address_space_t *as,
					struct mm_entry_allocator *al)
{
	arch_start_alloc_mm_entry_from_address_space(as, al);
}

static inline void
start_alloc_mm_entry_from_page_table(page_table_t *as,
				     struct mm_entry_allocator *al)
{
	arch_start_alloc_mm_entry_from_page_table(as, al);
}

static inline enum alloc_mm_entry_result
alloc_pte(struct mm_entry_allocator *al)
{
	return arch_alloc_pte(al);
}

static inline enum alloc_mm_entry_result
alloc_address_space_entry(struct mm_entry_allocator *al)
{
	return arch_alloc_address_space_entry(al);
}

static inline void
alloc_mm_entry_push_mem(struct mm_entry_allocator *al, uintptr_t mem)
{
	return arch_alloc_mm_entry_push_mem(al, mem);
}

/* sizeof mem > SIZEOF_ADDRESS_SPACE & aligned to ALIGNOF_ADDRESS_SPACE */
static inline address_space_t *
init_address_space(void *mem)
{
	return arch_init_address_space(mem);
}

/* sizeof mem > SIZEOF_PAGE_TABLE & aligned to ALIGNOF_PAGE_TABLE */
static inline page_table_t *
init_page_table(void *mem)
{
	return arch_init_page_table(mem);
}

static inline void
init_kernel_address_space(void)
{
	arch_init_kernel_address_space();
}

static inline pa_t
va2pa(va_t va)
{
	return arch_va2pa(va);
}

static inline va_t
pa2va(pa_t pa)
{
	return arch_pa2va(pa);
}

extern address_space_t kernel_address_space;

struct mem_region {
	pa_t begin;
	pa_t end;

	int num_all_page[NUM_PAGESIZE_LEVEL];
	npr_bitmap_t allocated[NUM_PAGESIZE_LEVEL];
};

struct mem_node {
	int num_region;
	struct mem_region *regions;
};

#define MAX_NODE 1
static const int num_node = MAX_NODE;

struct mem_node nodes[MAX_NODE];

#define DEFAULT_PAGESIZE_LEVEL 0

enum alloc_page_result {
	ALLOC_PAGE_SUCCESS,
};

struct alloc_page_info {
	int node;
	int region;
	int region_idx;
	enum alloc_page_result r;
	va_t addr;
};

void alloc_page(struct alloc_page_info *result,
		int node, int pagesize_level,
		int num_alloc, int num_page_per_alloc, int align_shift);
void free_page(struct alloc_page_info *allocs, int num_alloc, int num_page_per_alloc);

#endif
