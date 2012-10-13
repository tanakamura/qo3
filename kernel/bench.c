#include "kernel/bench.h"
#include "kernel/brk.h"
#include <emmintrin.h>
#include "kernel/intrinsics.h"
#include "kernel/event.h"
#include "kernel/net/r8169.h"
#include "kernel/smp.h"
#include <stdio.h>

//static unsigned char mwait_line[NUM_MAX_CPU][CACHE_LINE_WIDTH] __attribute__((aligned(CACHE_LINE_WIDTH)));
static unsigned char ready[NUM_MAX_CPU][CACHE_LINE_WIDTH] __attribute__((aligned(CACHE_LINE_WIDTH)));

static void
copy(void *p, void *q, int sz)
{
	__m128i *vp = (__m128i*)p;
	__m128i *vq = (__m128i*)q;

	for (int i=0; i<sz/(16*8); i++) {
		vp[i*8+0] = vq[i*8+0];
		vp[i*8+1] = vq[i*8+1];
		vp[i*8+2] = vq[i*8+2];
		vp[i*8+3] = vq[i*8+3];
		vp[i*8+4] = vq[i*8+4];
		vp[i*8+5] = vq[i*8+5];
		vp[i*8+6] = vq[i*8+6];
		vp[i*8+7] = vq[i*8+7];
	}
}

static void
store(void *p, int sz)
{
	__m128i *vp = (__m128i*)p;

	for (int i=0; i<sz/(16*8); i++) {
		vp[i*8+0] = _mm_setzero_si128();
		vp[i*8+1] = _mm_setzero_si128();
		vp[i*8+2] = _mm_setzero_si128();
		vp[i*8+3] = _mm_setzero_si128();
		vp[i*8+4] = _mm_setzero_si128();
		vp[i*8+5] = _mm_setzero_si128();
		vp[i*8+6] = _mm_setzero_si128();
		vp[i*8+7] = _mm_setzero_si128();
	}
}

static void
store_stream(void *p, int sz)
{
	__m128i *vp = (__m128i*)p;

	for (int i=0; i<sz/(16*8); i++) {
		_mm_stream_si128(&vp[i*8+0], _mm_setzero_si128());
		_mm_stream_si128(&vp[i*8+1], _mm_setzero_si128());
		_mm_stream_si128(&vp[i*8+2], _mm_setzero_si128());
		_mm_stream_si128(&vp[i*8+3], _mm_setzero_si128());
		_mm_stream_si128(&vp[i*8+4], _mm_setzero_si128());
		_mm_stream_si128(&vp[i*8+5], _mm_setzero_si128());
		_mm_stream_si128(&vp[i*8+6], _mm_setzero_si128());
		_mm_stream_si128(&vp[i*8+7], _mm_setzero_si128());
	}
}

static void
load(void *p, int sz)
{
	volatile __m128i *vp = (volatile __m128i*)p;

	for (int i=0; i<sz/(16*8); i++) {
		vp[i*8+0];
		vp[i*8+1];
		vp[i*8+2];
		vp[i*8+3];
		vp[i*8+4];
		vp[i*8+5];
		vp[i*8+6];
		vp[i*8+7];
	}
}

#define SIZE (8)

static char mem0_buffer[SIZE]__attribute__((aligned(16)));
static char mem1_buffer[SIZE]__attribute__((aligned(16)));
static char mem_ap_buffer0[NUM_MAX_CPU][SIZE]__attribute__((aligned(16)));
static char mem_ap_buffer1[NUM_MAX_CPU][SIZE]__attribute__((aligned(16)));

static void
mem_bench(void *mem0, void *mem1)
{
	int sz = SIZE;
	unsigned int begin, end;

	begin = rdtsc_lo();
	copy(mem0, mem1, sz);
	end = rdtsc_lo();
	printf("copy: %d\n", end-begin);

	begin = rdtsc_lo();
	copy(mem0, mem1, sz);
	end = rdtsc_lo();

	printf("copy: %d\n", end-begin);

	begin = rdtsc_lo();
	store(mem0, sz);
	end = rdtsc_lo();

	printf("store: %d\n", end-begin);

	begin = rdtsc_lo();
	store_stream(mem0, sz);
	end = rdtsc_lo();

	printf("store_stream: %d\n", end-begin);

	begin = rdtsc_lo();
	load(mem0, sz);
	end = rdtsc_lo();

	printf("load: %d\n", end-begin);
}

static void
r8169_bench(struct r8169_dev *r8169)
{
	int i;
	for (i=1; i<=2048; i*=2) {
		unsigned int begin, end;
		event_bits_t done = 0;

		begin = rdtsc_lo();
		r8169_tx_packet(r8169, mem0_buffer, i, 0, 
				&done, 1);

		wait_event_any(&done, 1);
		end = rdtsc_lo();

		printf("r8169 tx packet(%d): %d\n", i, end-begin);
	}
}

static void
watch_mem1(unsigned char *ptr, int expect, int c_state)
{
	while (1) {
		monitor(ptr, 0, 0);
		lfence();
		if (*ptr == expect) break;
		mwait(c_state, 0);
	}
}


static void
smp_bench(void)
{
	int cpus[NUM_MAX_CPU];
	int i, ncpu=0, c_state;
	void *mem0 = (void*)mem0_buffer;
	void *mem1 = (void*)mem1_buffer;
	int sz = SIZE;

	printf("available processors:");
	for (i=0; i<NUM_MAX_CPU; i++) {
		if (smp_table[i].flags & PROCESSOR_ENABLED) {
			printf(" %d",i);
			cpus[ncpu++] = i;
		}
	}
	puts("");

	for (i=0; i<ncpu; i++) {
		int cpu = cpus[i];
		ready[cpu][0] = 0;
		sfence();
		post_command_to_ap(cpu, AP_COMMAND_RUN_BENCH);
	}

	for (i=0; i<ncpu; i++) {
		int cpu = cpus[i];
		watch_mem1(&ready[cpu][0], 1, C_STATE(1,0));
	}

	puts("ready all cpus");

	for (c_state=1;c_state<5;c_state++){ 
		for (i=0; i<ncpu; i++) {
			unsigned int begin, end;
			int cpu = cpus[i];

			begin = rdtsc_lo();
			ready[cpu][0] = 2;
			watch_mem1(&ready[cpu][0], 3, C_STATE(c_state,0));
			end = rdtsc_lo();

			printf("mwait ping pong(%d-%d): %d\n", c_state, cpu, end-begin);
		}
	}

	for (i=0; i<ncpu; i++) {
		int cpu = cpus[i];
		unsigned char *watch_ptr = &ready[cpu][0];
		unsigned int begin, end;

		*watch_ptr = 4;
		begin = rdtsc_lo();
		copy(mem0, mem1, sz);
		watch_mem1(watch_ptr, 5, C_STATE(1,0));
		end = rdtsc_lo();
		printf("parallel copy(%d): %d\n", cpu, end-begin);

		*watch_ptr = 6;
		begin = rdtsc_lo();
		store_stream(mem0, sz);
		watch_mem1(watch_ptr, 7, C_STATE(1,0));
		end = rdtsc_lo();

		printf("parallel store_stream(%d): %d\n", cpu, end-begin);

		*watch_ptr = 8;
		begin = rdtsc_lo();
		load(mem0, sz);
		watch_mem1(watch_ptr, 9, C_STATE(1,0));
		end = rdtsc_lo();

		printf("parallel load(%d): %d\n", cpu, end-begin);
	}

	{
		unsigned int begin, end;

		begin = rdtsc_lo();
		for (i=0; i<ncpu; i++) {
			int cpu = cpus[i];
			ready[cpu][0] = 4;
		}
		copy(mem0, mem1, sz);
		for (i=0; i<ncpu; i++) {
			int cpu = cpus[i];
			watch_mem1(&ready[cpu][0], 5, C_STATE(1,0));
		}
		end = rdtsc_lo();
		printf("parallel copy(all): %d\n", end-begin);

		begin = rdtsc_lo();
		for (i=0; i<ncpu; i++) {
			int cpu = cpus[i];
			ready[cpu][0] = 6;
		}
		store_stream(mem0, sz);
		for (i=0; i<ncpu; i++) {
			int cpu = cpus[i];
			watch_mem1(&ready[cpu][0], 7, C_STATE(1,0));
		}
		end = rdtsc_lo();

		printf("parallel store_stream(all): %d\n", end-begin);

		begin = rdtsc_lo();
		for (i=0; i<ncpu; i++) {
			int cpu = cpus[i];
			ready[cpu][0] = 8;
		}
		load(mem0, sz);
		for (i=0; i<ncpu; i++) {
			int cpu = cpus[i];
			watch_mem1(&ready[cpu][0], 9, C_STATE(1,0));
		}
		end = rdtsc_lo();

		printf("parallel load(all): %d\n", end-begin);
	}

}

void
run_bench(struct r8169_dev *r8169)
{
	void *mem0 = (void*)mem0_buffer;
	void *mem1 = (void*)mem1_buffer;

	mem_bench(mem0, mem1);
	r8169_bench(r8169);
	smp_bench();
}


void
run_bench_ap(int apic_id)
{
	int c_state;
	unsigned char *watch_ptr = &ready[apic_id][0];
	void *mem0 = (void*)mem_ap_buffer0[apic_id];
	void *mem1 = (void*)mem_ap_buffer1[apic_id];
	int sz = SIZE;

	ready[apic_id][0] = 1;

	for (c_state=1; c_state<5; c_state++) {
		watch_mem1(&ready[apic_id][0], 2, C_STATE(c_state,0));
		ready[apic_id][0] = 3;
		sfence();
	}

	watch_mem1(watch_ptr, 4, C_STATE(1,0));
	copy(mem0, mem1, sz);
	*watch_ptr = 5;

	watch_mem1(watch_ptr, 6, C_STATE(1,0));
	store_stream(mem0, sz);
	*watch_ptr = 7;

	watch_mem1(watch_ptr, 8, C_STATE(1,0));
	load(mem0, sz);
	*watch_ptr = 9;



	watch_mem1(watch_ptr, 4, C_STATE(1,0));
	copy(mem0, mem1, sz);
	*watch_ptr = 5;

	watch_mem1(watch_ptr, 6, C_STATE(1,0));
	store_stream(mem0, sz);
	*watch_ptr = 7;

	watch_mem1(watch_ptr, 8, C_STATE(1,0));
	load(mem0, sz);
	*watch_ptr = 9;
}
