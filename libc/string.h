#ifndef QO3_STRING_H
#define QO3_STRING_H

#include <stddef.h>

size_t strlen(const char *str);
char *strcat(char *dest, const char *src);

void *memset(void *p, int c, size_t len);
void *memcpy(void *p, const void *src, size_t len);

char *strcpy(char *dest, const char *src);
char *strncpy(char *dest, const char *src, size_t n);

int strcmp(const char *s1, const char *s2);
int strncmp(const char *s1, const char *s2, size_t n);

int memcmp(const void *s1, const void *s2, size_t n);

#endif
