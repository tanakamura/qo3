#ifndef QO3_KERNEL_INTRINSICS_H
#define QO3_KERNEL_INTRINSICS_H

#include <stdint.h>

#define rdmsr(c,a,d)                                            \
        __asm__ __volatile__ ("rdmsr"                           \
                              :"=a"(a),"=d"(d)                  \
                              :"c"(c))

#define rdmsrll(c,ll)                                            \
        __asm__ __volatile__ ("rdmsr"                            \
                              :"=A"(ll)                          \
                              :"c"(c))


#define wrmsr(c,a,d)                                            \
        __asm__ __volatile__ ("rdmsr"                           \
			      :					\
                              :"a"(a),"d"(d),"c"(c))

#define wrmsrll(c,ll)                                            \
        __asm__ __volatile__ ("rdmsr"                            \
			      :					 \
                              :"A"(ll),"c"(c))


#define cpuid(a,ao,b,c,d) \
        __asm__ __volatile__ ("cpuid"                           \
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



#define hlt()                                   \
        __asm__ __volatile__ ("hlt")

/* enforce io */
#define eieio()

/* force memory access to c compiler */
#define cbarrier(v) __asm__ __volatile__(""::"r"(v))


#define lfence() __asm__ __volatile__("lfence":::"memory")
#define sfence() __asm__ __volatile__("sfence":::"memory")

#endif
