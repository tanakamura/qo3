#define NPR_BITTREE_DUMP
#include "../bittree.h"
#include "../bittree.c"
#include <assert.h>
#include <x86intrin.h>
#include <string.h>

int
main()
{
	int num_bits = 110;
	int size = npr_bittree_byte_size(num_bits);
	int bit;
	int i;
	void *data;
	struct npr_bittree tree;
	int b, e;

	assert(size == (3+4)*4);

	data = malloc(size);
	npr_bittree_init(&tree, num_bits);

	assert(tree.offsets[0] == 2);
	assert(tree.offsets[1] == 3);

	npr_bittree_set_all(&tree, data, num_bits);

	npr_bittree_dump(&tree, data, num_bits);
	puts("");

	for (i=0; i<65; i++) {
		b = __rdtsc();
		bit = npr_bittree_get(&tree, data);
		e = __rdtsc();
	}
	printf("%d\n", e-b);


	npr_bittree_dump(&tree, data, num_bits);
	puts("");

	npr_bittree_set(&tree, data, 80);

	npr_bittree_dump(&tree, data, num_bits);
	puts("");

}
