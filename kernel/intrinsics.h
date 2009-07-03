#ifndef QO3_KERNEL_INTRINSICS_H
#define QO3_KERNEL_INTRINSICS_H

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

#define hlt()                                   \
        __asm__ __volatile__ ("hlt")

#define wiob()

/* force memory access to c compiler */
#define cbarrier(v) __asm__ __volatile__(""::"r"(v))
#define lfence() __asm__ __volatile__("lfence":::"memory")
#define sfence() __asm__ __volatile__("sfence":::"memory")

#endif
