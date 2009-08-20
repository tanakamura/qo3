#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include "../npr/printf.h"
#include "../npr/printf-format.h"

int
vsnprintf(char *str, size_t size, const char *format, va_list ap)
{
	struct npr_printf_format fmt[16];
	struct npr_printf_arg args[16];
	int num_fmt;
	struct npr_printf_state st;
	int is_fini, out_size;
	struct npr_printf_build_format_error error;

	num_fmt = npr_printf_build_format(fmt, 16, format, strlen(format), &error);

	if ((num_fmt < 0) || (num_fmt >= 16)) {
		while (1);
	}
	npr_printf_build_varg(args, fmt, num_fmt, ap);

	npr_sprintf_start(&st);
	out_size = npr_sprintf(&st, str, size-1, fmt, num_fmt, args, &is_fini);

	if (! is_fini)
		return -1;

	str[out_size] = '\0';

	return out_size;
}

int
snprintf(char *str, size_t size, const char *format, ...)
{
	int ret;
	va_list ap;
	va_start(ap, format);
	ret = vsnprintf(str, size, format, ap);
	va_end(ap);
	return ret;
}

int
printf(const char *format, ...)
{
	int ret;
	va_list ap;
	va_start(ap, format);
	ret = vprintf(format, ap);
	va_end(ap);
	return ret;
}
