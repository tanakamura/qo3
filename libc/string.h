#ifndef QO3_STRING_H
#define QO3_STRING_H

#include <stddef.h>

size_t strlen(const char *str);
void *memset(void *p, int c, size_t len);
char *strcpy(char *dest, const char *src);
int strcmp(const char *s1, const char *s2);
int memcmp(const void *s1, const void *s2, size_t n);

#endif
