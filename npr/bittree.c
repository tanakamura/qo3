#include <x86intrin.h>
#include <string.h>

#include "bittree.h"
#include "align.h"
#include "static-assert.h"

int
npr_bittree_byte_size(unsigned int num_bits)
{
	unsigned int l1_size, l2_size, l3_size;

	NPR_STATIC_ASSERT(BITTREE_DEPTH == 4);

	l3_size = NPR_CEIL_DIV(num_bits, BITTREE_WORD_NUM_BITS);
	l2_size = NPR_CEIL_DIV(l3_size, BITTREE_WORD_NUM_BITS);
	l1_size = NPR_CEIL_DIV(l2_size, BITTREE_WORD_NUM_BITS);

	return (1 + l1_size + l2_size + l3_size) * sizeof(npr_bittree_bits_t);
}

void
npr_bittree_init(struct npr_bittree *tree,
		 unsigned int num_bits)
{
	unsigned int l1_size, l2_size, l3_size;

	NPR_STATIC_ASSERT(BITTREE_DEPTH == 4);

	l3_size = NPR_CEIL_DIV(num_bits, BITTREE_WORD_NUM_BITS);
	l2_size = NPR_CEIL_DIV(l3_size, BITTREE_WORD_NUM_BITS);
	l1_size = NPR_CEIL_DIV(l2_size, BITTREE_WORD_NUM_BITS);

	/* l0_size = 1 */
	tree->offsets[0] = 1 + l1_size;
	tree->offsets[1] = 1 + l1_size + l2_size;
}

void
npr_bittree_set(struct npr_bittree *t,
		void *buffer,
		unsigned int idx)
{
	unsigned int l2_off;
	unsigned int l3_off;

	unsigned int word_pos, bit_pos;
	npr_bittree_bits_t *p = (npr_bittree_bits_t*)buffer;

	NPR_STATIC_ASSERT(BITTREE_DEPTH == 4);

	l2_off = t->offsets[0];
	l3_off = t->offsets[1];

	bit_pos = idx % BITTREE_WORD_NUM_BITS;
	word_pos = idx / BITTREE_WORD_NUM_BITS;

	p[l3_off + word_pos] |= 1L<<bit_pos;

	bit_pos = word_pos % BITTREE_WORD_NUM_BITS;
	word_pos = word_pos / BITTREE_WORD_NUM_BITS;

	p[l2_off + word_pos] |= 1L<<bit_pos;

	bit_pos = word_pos % BITTREE_WORD_NUM_BITS;
	word_pos = word_pos / BITTREE_WORD_NUM_BITS;

	p[1 + word_pos] |= 1L<<bit_pos;

	bit_pos = word_pos % BITTREE_WORD_NUM_BITS;

	p[0] |= 1L<<bit_pos;
}

int
npr_bittree_get(struct npr_bittree *t,
		void *buffer)
{
	unsigned int l0_pos;
	unsigned int l1_pos;
	unsigned int l2_pos,l2_off;
	unsigned int l3_pos,l3_off;
	int ret;
	int nb = BITTREE_WORD_NUM_BITS;

	npr_bittree_bits_t b0, b1, b2, b3;
	npr_bittree_bits_t *p = (npr_bittree_bits_t*)buffer;

	NPR_STATIC_ASSERT(BITTREE_DEPTH == 4);

	l2_off = t->offsets[0];
	l3_off = t->offsets[1];

#ifdef __x86_64__
	l0_pos = __bsrq(b0 = p[0]);
	l1_pos = __bsrq(b1 = p[1 + l0_pos]);
	l2_pos = __bsrq(b2 = p[l2_off + l1_pos]);
	l3_pos = __bsrq(b3 = p[l3_off + l2_pos]);
#elif defined(__i386__)
	l0_pos = __bsrd(b0 = p[0]);
	l1_pos = __bsrd(b1 = p[1 + l0_pos]);
	l2_pos = __bsrd(b2 = p[l2_off + l1_pos]);
	l3_pos = __bsrd(b3 = p[l3_off + l2_pos]);
#else
#error "help me!"
#endif
	ret = ((l0_pos*nb + l1_pos)*nb + l2_pos)*nb + l3_pos;

	b3 &= ~(1L<<l3_pos);
	p[l3_off + l2_pos] = b3;

	b2 &= ~(((npr_bittree_bits_t)(b3==0))<<l2_pos);
	p[l2_off + l1_pos] = b2;

	b1 &= ~(((npr_bittree_bits_t)(b2==0))<<l1_pos);
	p[1 + l0_pos] = b1;

	b0 &= ~(((npr_bittree_bits_t)(b1==0))<<l0_pos);
	p[0] = b0;

	return ret;
}

void
npr_bittree_set_all(struct npr_bittree *t,
		    void *buffer,
		    unsigned int num_bits)
{
	unsigned int l1_size_word, l2_size_word, l3_size_word;
	unsigned int l0_size_byte, l1_size_byte, l2_size_byte, l3_size_byte;
	unsigned int l0_size_bit, l1_size_bit, l2_size_bit, l3_size_bit;

	npr_bittree_bits_t *p = (npr_bittree_bits_t*)buffer;

	NPR_STATIC_ASSERT(BITTREE_DEPTH == 4);

	l3_size_word = NPR_CEIL_DIV(num_bits, BITTREE_WORD_NUM_BITS);
	l3_size_byte = num_bits / 8U;
	l3_size_bit = num_bits % 8U;

	memset(p + t->offsets[1], 0xff, l3_size_byte);
	*(char*)(p + t->offsets[1] + l3_size_byte) = (1<<(l3_size_bit))-1;

	l2_size_word = NPR_CEIL_DIV(l3_size_word, BITTREE_WORD_NUM_BITS);
	l2_size_byte = l3_size_word / 8U;
	l2_size_bit = l3_size_word % 8U;

	memset(p + t->offsets[0], 0xff, l2_size_byte);
	*(char*)(p + t->offsets[0] + l2_size_byte) = (1<<(l2_size_bit))-1;

	l1_size_word = NPR_CEIL_DIV(l2_size_word, BITTREE_WORD_NUM_BITS);
	l1_size_byte = l2_size_word / 8U;
	l1_size_bit = l2_size_word % 8U;

	memset(p + 1, 0xff, l1_size_byte);
	*(char*)(p + 1 + l2_size_byte) = (1<<(l1_size_bit))-1;


	l0_size_byte = l1_size_word / 8U;
	l0_size_bit = l1_size_word % 8U;

	memset(p, 0xff, l0_size_byte);
	*(char*)(p + l0_size_byte) = (1<<(l0_size_bit))-1;
}

#ifdef NPR_BITTREE_DUMP
static void
dump_bits(npr_bittree_bits_t *p, int n)
{
	int i, j;
	for (i=0; i<n; i++) {
		for (j=0; j<BITTREE_WORD_NUM_BITS; j++) {
			if (p[i] & (1L<<j)) {
				putchar('1');
			} else {
				putchar('0');
			}
		}
	}
}

void
npr_bittree_dump(struct npr_bittree *tree,
		 void *buffer,
		 unsigned int num_bits)
{
	unsigned int l1_size, l2_size, l3_size, l2_off, l3_off;
	npr_bittree_bits_t *p = (npr_bittree_bits_t*)buffer;

	NPR_STATIC_ASSERT(BITTREE_DEPTH == 4);

	l3_size = NPR_CEIL_DIV(num_bits, BITTREE_WORD_NUM_BITS);
	l2_size = NPR_CEIL_DIV(l3_size, BITTREE_WORD_NUM_BITS);
	l1_size = NPR_CEIL_DIV(l2_size, BITTREE_WORD_NUM_BITS);

	l2_off = tree->offsets[0];
	l3_off = tree->offsets[1];

	dump_bits(p, 1);
	putchar('\n');
	dump_bits(p+1, l1_size);
	putchar('\n');
	dump_bits(p+l2_off, l2_size);
	putchar('\n');
	dump_bits(p+l3_off, l3_size);
	putchar('\n');

}
#endif
