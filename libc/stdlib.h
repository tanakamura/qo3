#ifndef QO3_STDLIB_H
#define QO3_STDLIB_H

#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

int atoi(const char *nptr);
unsigned long strtoul(const char *ptr,char **endptr,int base);

#ifdef __cplusplus
}
#endif

#endif
