#ifndef QO3_STDIO_H
#define QO3_STDIO_H

#include <stddef.h>
#include <stdarg.h>

int vsnprintf(char *str, size_t size, const char *format, va_list ap);
int snprintf(char *str, size_t size, const char *format, ...);
int printf(const char *format, ...);
int vprintf(const char *format, va_list ap);
int puts(const char *str);
int putchar(int c);

#endif
