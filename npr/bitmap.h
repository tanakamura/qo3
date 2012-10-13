#ifndef NPR_BITMAP_H
#define NPR_BITMAP_H

#include "npr/align.h"

typedef unsigned long npr_bitmap_bits_t;
typedef npr_bitmap_bits_t *npr_bitmap_t;

#define BITMAP_NUM_BITS (sizeof(npr_bitmap_t)*8U)
#define BYTESIZE_OF_BITMAP(n) NPR_ALIGN_UP(((n)+7U)/8U, sizeof(npr_bitmap_bits_t))
#define WORDSIZE_OF_BITMAP(n) ((((n)+(BITMAP_NUM_BITS-1U))/BITMAP_NUM_BITS))

void npr_bmp_clear_all(npr_bitmap_t bits, int len);
static inline int
npr_bmp_p(npr_bitmap_t bits, unsigned int idx)
{
	unsigned int wordpos = idx/(sizeof(npr_bitmap_t)*8U);
	unsigned int bitpos = idx%(sizeof(npr_bitmap_t)*8U);

	return bits[wordpos]&(1<<bitpos);
}

static inline int
npr_bitmap_ffs(npr_bitmap_t bmp, unsigned int length)
{
    unsigned int n = WORDSIZE_OF_BITMAP(length);
    unsigned int i;
    for (i=0; i<n; i++) {
        if (bmp[i]) {
            unsigned int ret = (__builtin_ffsl(bmp[i])-1) + i*BITMAP_NUM_BITS;
            if (ret > length)
                return -1;
            return ret;
        }
    }
    return -1;
}

static inline void
npr_bitmap_clear(npr_bitmap_t bmp, unsigned int idx)
{
    unsigned int wordpos = idx/(sizeof(npr_bitmap_bits_t)*8U);
    unsigned int bitpos = idx%(sizeof(npr_bitmap_bits_t)*8U);

    bmp[wordpos] &= ~(1<<bitpos);
}

static inline void
npr_bitmap_set(npr_bitmap_t bmp, unsigned int idx)
{
    unsigned int wordpos = idx/(sizeof(npr_bitmap_bits_t)*8U);
    unsigned int bitpos = idx%(sizeof(npr_bitmap_bits_t)*8U);

    bmp[wordpos] |= (1<<bitpos);
}


static inline void
npr_bitmap_clear_all(npr_bitmap_t bits, unsigned int len)
{
	unsigned int n = WORDSIZE_OF_BITMAP(len);
	unsigned int i;
	for (i=0; i<n; i++) {
		bits[i] = (npr_bitmap_bits_t)0;
	}
}

#endif
