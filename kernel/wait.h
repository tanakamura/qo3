#ifndef QO3_KERNEL_WAIT_H
#define QO3_KERNEL_WAIT_H

void wait_setup(void);
void wait_msec(unsigned int msec);
#define wait_sec(n) wait_msec((n)*1000)

#endif
