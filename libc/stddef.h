#ifndef QO3_STDDEF_H
#define QO3_STDDEF_H

#ifdef __cplusplus
extern "C" {
#endif

#ifdef __x86_64__
typedef unsigned long size_t;
#else
typedef unsigned int size_t;
#endif
#define NULL (0)

#ifdef __cplusplus
}
#endif

#endif
