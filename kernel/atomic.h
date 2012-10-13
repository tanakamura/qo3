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
	asm volatile("lock cmpxcgh %3\n"
		     :
		     
}

#endif
