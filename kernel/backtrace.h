#ifndef QO3_KERNEL_BACKTRACE_H
#define QO3_KERNEL_BACKTRACE_H

#include "kernel/qo3-types.h"

void dump_backtrace(reg_t start_rbp, int depth, int indent);

#endif
