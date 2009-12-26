#ifndef QO3_KERNEL_INTRINSICS_H
#define QO3_KERNEL_INTRINSICS_H

#include <stdint.h>

#define rdmsr(c,a,d)						\
	__asm__ __volatile__ ("rdmsr"				\
			      :"=a"(a),"=d"(d)			\
			      :"c"(c))

#define rdmsrll(c,ll)						 \
	__asm__ __volatile__ ("rdmsr"				 \
			      :"=A"(ll)				 \
			      :"c"(c))


#define wrmsr(c,a,d)						\
	__asm__ __volatile__ ("rdmsr"				\
			      :					\
			      :"a"(a),"d"(d),"c"(c))

#define wrmsrll(c,ll)						 \
	__asm__ __volatile__ ("rdmsr"				 \
			      :					 \
			      :"A"(ll),"c"(c))


#define cpuid(a,ao,b,c,d) \
	__asm__ __volatile__ ("cpuid"				\
			      :"=a"(ao),"=b"(b),"=c"(c),"=d"(d) \
			      :"a"(a))

#define mwait(a,c)				\
	__asm__ __volatile__ ("mwait"		\
			      :			\
			      :"a"(a), "c"(c))

#define monitor(a,c,d)					\
	__asm__ __volatile__ ("monitor"			\
			      :				\
			      :"a"(a), "c"(c), "d"(d))



static inline unsigned char inb(unsigned short port)
{
	unsigned char a;
	__asm__ __volatile__ ("inb %1, %0": "=a"(a): "Nd"(port));
	return a;
}
static inline void outb(unsigned short port, unsigned char val)
{
	__asm__ __volatile__ ("outb %0, %1":: "a"(val), "Nd"(port));
}

static inline uint16_t inw(unsigned short port)
{
	uint16_t a;
	__asm__ __volatile__ ("inw %1, %0": "=a"(a): "Nd"(port));
	return a;
}
static inline void outw(unsigned short port, uint16_t val)
{
	__asm__ __volatile__ ("outw %0, %1":: "a"(val), "Nd"(port));
}
static inline uint32_t inl(unsigned short port)
{
	uint32_t a;
	__asm__ __volatile__ ("inl %1, %0": "=a"(a): "Nd"(port));
	return a;
}
static inline void outl(unsigned short port, uint32_t val)
{
	__asm__ __volatile__ ("outl %0, %1":: "a"(val), "Nd"(port));
}

#define in8 inb
#define in16 inw
#define in32 inl
#define out8 outb
#define out16 outw
#define out32 outl

#define hlt()					\
	__asm__ __volatile__ ("hlt")

/* enforce io */
#define eieio()

/* force memory access to c compiler */
#define cbarrier(v) __asm__ __volatile__(""::"r"(v))

#define lfence() __asm__ __volatile__("lfence":::"memory")
#define sfence() __asm__ __volatile__("sfence":::"memory")
#define mfence() __asm__ __volatile__("mfence":::"memory")

static inline uint16_t
movbe_load16(uint16_t *addr)
{
	uint16_t r;
	__asm__("movbew %1,%0"
		:"=r"(r)
		:"m"(*addr));
	return r;
}
static inline uint32_t
movbe_load32(uint32_t *addr)
{
	uint32_t r;
	__asm__("movbel %1,%0"
		:"=r"(r)
		:"m"(*addr));
	return r;
}

static inline void
movbe_store16(uint16_t *addr, uint16_t v)
{
	__asm__ __volatile__("movbew %1,%0"
			     :"=m"(*addr)
			     :"r"(v));
}
static inline void
movbe_store32(uint32_t *addr, uint32_t v)
{
	__asm__ __volatile__("movbel %1,%0"
			     :	"=m"(*addr)
			     : "r"(v));
}

#endif
