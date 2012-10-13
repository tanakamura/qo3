#include <string.h>
#include <npr/printf.h>
#include <npr/printf-format.h>
#include <ns16550.h>
#include <stdio.h>
#include "kernel/fatal.h"
#include "acpi.h"


#define BUFSIZE 256

int
vprintf(const char *format,
	va_list ap)
{
	char buffer[BUFSIZE];
	struct npr_printf_format fmt[32];
	struct npr_printf_arg args[32];
	int num_fmt;
	struct npr_printf_state st;
	int is_fini, out_size;
	struct npr_printf_build_format_error error;

	num_fmt = npr_printf_build_format(fmt, 32, format, strlen(format), &error);

	if (num_fmt < 0) {
		switch (error.code) {
		case NPR_PRINTF_BUILD_FORMAT_TOO_MANY_FORMATS:
			puts("too many format for printf");
			break;

		case NPR_PRINTF_BUILD_FORMAT_INVALID_FORMAT:
			printf("printf format error @ %c(%d)(%s)\n",
			       format[error.idx],
			       error.idx,
			       format);
			break;
		}
		fatal();
	} else if (num_fmt >= 32) {
		fatal();
	}
	npr_printf_build_varg(args, fmt, num_fmt, ap);

	npr_sprintf_start(&st);
	out_size = npr_sprintf(&st, buffer, BUFSIZE, fmt, num_fmt, args, &is_fini);

	if (! is_fini) {
		puts("printf error");
		return -1;
	}

	ns16550_write_text_poll(buffer, out_size);

	return out_size;
}


int
puts(const char *str)
{
	char crlf[] = "\r\n";
	int len = strlen(str);

	ns16550_write_poll(str, len);
	ns16550_write_poll(crlf, 2);

	return len+2;
}

int
putchar(int c)
{
	char buf[1];
	buf[0] = c;
	ns16550_write_poll(buf, 1);

	return c;
}
