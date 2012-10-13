#ifndef NPR_BITTREE_H
#define NPR_BITTREE_H

typedef unsigned long npr_bittree_bits_t;

#define BITTREE_DEPTH 4
#define BITTREE_WORD_NUM_BITS (sizeof(npr_bittree_bits_t)*8U)
#define BITTREE_MAX_SIZE (BITTREE_WORD_NUM_BITS * BITTREE_WORD_NUM_BITS * \
			  BITTREE_WORD_NUM_BITS * BITTREE_WORD_NUM_BITS)

struct npr_bittree {
	/* level0 = 0
	 * level1 = 1 */
	unsigned int offsets[BITTREE_DEPTH-2];
};

int npr_bittree_byte_size(unsigned int num_bits);

void npr_bittree_init(struct npr_bittree *tree,
		      unsigned int num_bits);

/* ffs + clear
 * return negative if failed */
int npr_bittree_get(struct npr_bittree *t,
		    void *buffer);

void npr_bittree_set(struct npr_bittree *t,
		     void *buffer,
		     unsigned int idx);

void npr_bittree_clear(struct npr_bittree *t,
		       void *buffer,
		       unsigned int idx);

int npr_bittree_p(struct npr_bittree *t,
		  void *buffer,
		  unsigned int idx);

#define NPR_BITTREE_HAVE_BITS(buffer) ((*(npr_bittree_bits_t*)(buffer))!=0)

void npr_bittree_set_all(struct npr_bittree *t,
			 void *buffer,
			 unsigned int num_bits);

/*
 * clear_all() = { memset(buffer, 0, npr_bittree_bytesize(num_bits)); }
 */

#ifdef NPR_BITTREE_DUMP
void npr_bittree_dump(struct npr_bittree *tree,
		      void *buffer,
		      unsigned int num_bits);
#endif

#endif
