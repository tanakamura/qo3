#ifndef QO3_KERNEL_ATOMIC_H
#define QO3_KERNEL_ATOMIC_H

typedef unsigned int atomic_t;

#ifdef __x86_64__
typedef uint64_t atomic_pointer_flag_pair_flag_t;
#endif


struct __attribute__((aligned(16))) atomic_pointer_flag_pair {
	atomic_pointer_flag_pair_flag_t flags;
	void *ptr;
};

static inline int
atomic_pointer_flag_pair_cas(struct atomic_pointer_flag_pair *val,
			     atomic_pointer_flag_pair_flag_t old_flags,
			     void *old_pointer,
			     atomic_pointer_flag_pair_flag_t new_flags,
			     void *new_pointer)
{
    return 0;
}

#define compiler_wmb() __asm__ __volatile__("":::"memory")
#define compiler_rmb() __asm__ __volatile__("":::"memory")
#define compiler_mb() __asm__ __volatile__("":::"memory")

typedef uint32_t spinlock_t;

static inline void
spinlock_init(spinlock_t *addr)
{
	*addr = 0;
}

static inline void
spinlock(spinlock_t *addr)
{
    while (1) {
	uint32_t val = *addr;
	if (val == 0) {
	    uint32_t result;
	    __asm__ __volatile__("lock; cmpxchg %1, %2\n"
				 : "=a" (result)
				 : "r" (1), "m" (*addr), "0" (0)
				 : "memory");
	    if (result == 0) {
		break;
	    }
	}
	compiler_rmb();
    }
}

static inline void
spinunlock(spinlock_t *addr)
{
	*addr = 0;
	compiler_wmb();
}

static inline void cli()
{
	__asm__ __volatile__ ("cli":::"memory");
}
static inline void sti()
{
	__asm__ __volatile__ ("sti":::"memory");
}

typedef uint64_t eflags_t;

static inline eflags_t
get_flags()
{
	eflags_t flags;

	__asm__ __volatile__ ("pushf; pop %0"
			      :"=rm"(flags)
			      :
			      :"memory");

	return flags;
}

static inline void
set_flags(eflags_t flags)
{
	__asm__ __volatile__ ("push %0; popf"
			      :
			      :"g"(flags)
			      :"memory");
}

static inline eflags_t
spinlock_and_disable_int_local(spinlock_t *addr)
{
	eflags_t flags = get_flags();
	cli();
	while (1) {
		uint32_t val = *addr;

		if (val == 0) {
			uint32_t result;
			__asm__ __volatile__("cli\n\t"
					     "lock; cmpxchg %1, %2\n"
					     : "=a" (result)
					     : "r" (1), "m" (*addr), "0" (0)
					     : "memory");
			if (result == 0) {
				break;
			}
		}
		compiler_rmb();
	}

	return flags;
}

static inline void
spinunlock_and_restore_int_local(spinlock_t *addr, eflags_t flags)
{
	*addr = 0;
	set_flags(flags);
}



#endif
