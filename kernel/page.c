#include "kernel/page.h"

struct mem_node mem_nodes[MAX_NODE];

void
alloc_page(struct alloc_page_info *result,
	   int node, int pagesize_level,
	   int num_alloc, int num_page_per_alloc, int align_shift)
{
	
}
