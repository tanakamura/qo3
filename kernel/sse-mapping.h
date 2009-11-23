#ifndef QO3_KERNEL_SSE_MAPPING_H
#define QO3_KERNEL_SSE_MAPPING_H

#include <emmintrin.h>

static inline
__m128i sum_32(__m128i x)
{
	x = _mm_add_epi32(x, _mm_slli_si128(x, 8));
	x = _mm_add_epi32(x, _mm_slli_si128(x, 4));
	return x;
}

static inline
__m128i sum_16(__m128i x)
{
	x = _mm_add_epi16(x, _mm_slli_si128(x, 8));
	x = _mm_add_epi16(x, _mm_slli_si128(x, 4));
	x = _mm_add_epi16(x, _mm_slli_si128(x, 2));
	return x;
}

#endif
