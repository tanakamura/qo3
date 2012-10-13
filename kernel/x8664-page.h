#ifndef X8664_PTE_ALLOCATOR_H
#define X8664_PTE_ALLOCATOR_H

#include <kernel/qo3-types.h>

/*
 * pml4 table -> pdp table -> page dir -> page table -> page -> physical
 *
 * 262TB / pml4 table
 * 512GB / page directory pointer table
 * 1GB   / page directory
 * 2MB   / page table
 * 4096  / page
 */

typedef unsigned char pagedata_entry_t[8];

typedef pagedata_entry_t pte_t;
typedef pagedata_entry_t pde_t;
typedef pagedata_entry_t pdp_entry_t;
typedef pagedata_entry_t pml4_entry_t;

typedef pml4_entry_t address_space_entry_t;

typedef unsigned char pml4_table_t[4096];
typedef unsigned char pdp_table_t[4096];
typedef unsigned char page_directory_t[4096];
typedef unsigned char x86_page_table_t[4096];

typedef pml4_table_t address_space_t;
typedef pdp_table_t page_table_t;

extern pml4_table_t startup_pml4;
extern pdp_table_t startup_pdp;
extern page_directory_t startup_pdir;
extern x86_page_table_t startup_page_table;

#define ALIGNOF_ADDRESS_SPACE 4096
#define ALIGNOF_ADDRESS_SPACE_SHIFT 12
#define SIZEOF_ADDRESS_SPACE 4096

#define ALIGNOF_PAGE_TABLE 4096
#define ALIGNOF_PAGE_TABLE_SHIFT 12
#define SIZEOF_PAGE_TABLE 4096

enum arch_pte_allocator_depth {
	ARCH_PTE_ALLOCATOR_DEPTH_PML4,
	ARCH_PTE_ALLOCATOR_DEPTH_PDP,
	ARCH_PTE_ALLOCATOR_DEPTH_PDE,
};

struct arch_mm_entry_allocator {
	enum arch_pte_allocator_depth depth;

	pml4_entry_t *pml4_entry;
	pdp_entry_t *pdp_entry;
	pde_t *pde;
	pte_t *pte;
};

#define X86_MMU_ENTRY_P (1<<0)
#define X86_MMU_ENTRY_READ_WRITE (1<<1)
#define X86_MMU_ENTRY_READ_ONLY (0<<1)
#define X86_MMU_ENTRY_USER (1<<2)
#define X86_MMU_ENTRY_SUPERVISOR (0<<2)

#define NUM_PAGESIZE_LEVEL 3
#define PAGESIZE_LEVEL0_SIZE 4096U
#define PAGESIZE_LEVEL0_SHIFT 12U

#define PAGESIZE_LEVEL1_SIZE (2048U*1024U)
#define PAGESIZE_LEVEL1_SHIFT 21U

#define PAGESIZE_LEVEL2_SIZE (1U*1024U*1024U*1024U)
#define PAGESIZE_LEVEL2_SHIFT 30

/* PAT(7) + PWT(3) + PCD(4) */
#define X86_PATINDEX_TO_MMU_ENTRY(x) ((((x)&3)<<3)|(((x)&4)<<5))

#define X86_MMU_ENTRY_A (1<<5)
#define X86_MMU_ENTRY_DIRTY (1<<6)
#define X86_MMU_ENTRY_2MB (1<<7)

struct mm_entry_allocator ;

void arch_init_kernel_address_space(void);
void arch_init_mem_region(void);

void arch_start_alloc_mm_entry_from_pml4_table(pml4_table_t *as,
					       struct mm_entry_allocator *al);
void arch_start_alloc_mm_entry_from_pdp_table(pdp_table_t *as,
					      struct mm_entry_allocator *al);

void arch_alloc_mm_entry_push_mem(struct mm_entry_allocator *al, va_t mem);

enum alloc_mm_entry_result arch_alloc_pte(struct mm_entry_allocator *al);
enum alloc_mm_entry_result arch_alloc_pml4_entry(struct mm_entry_allocator *al);
pml4_table_t *arch_init_pml4_table(void *mem);
pdp_table_t *arch_init_pdp_table(void *mem);
pa_t arch_va2pa(va_t va);
va_t arch_pa2va(pa_t pa);

#define arch_start_alloc_mm_entry_from_address_space arch_start_alloc_mm_entry_from_pml4_table
#define arch_alloc_address_space_entry arch_alloc_pml4_entry
#define arch_start_alloc_mm_entry_from_page_table arch_start_alloc_mm_entry_from_pdp_table
#define arch_init_address_space arch_init_pml4_table
#define arch_init_page_table arch_init_pdp_table

#endif
