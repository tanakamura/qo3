#ifndef QO3_KERNEL_ATOMIC_H
#define QO3_KERNEL_ATOMIC_H

typedef uintptr_t spinlock_t;

/* todo : multi processor */
static inline void
spinlock_and_disable_int_self(spinlock_t lock)
{
	__asm__ __volatile__ ("cli");
}

static inline void
spinlock(spinlock_t lock)
{
	(void)lock;
}


static inline void
spinunlock_and_enable_int_self(spinlock_t lock)
{
	__asm__ __volatile__ ("sti");
}

static inline void
spinunlock(spinlock_t lock)
{
	(void)lock;
}


static inline void spinlock_init(spinlock_t *lock) {
	*lock = 0;
}

#endif
