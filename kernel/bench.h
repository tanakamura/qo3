#ifndef QO3_BENCH_H
#define QO3_BENCH_H

struct r8169_dev;

void run_bench(struct r8169_dev *r8169);
void run_bench_ap(int apic_id);

#endif
