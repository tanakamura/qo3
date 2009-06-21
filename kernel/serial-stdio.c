#include <string.h>
#include <npr/printf.h>
#include <npr/printf-format.h>
#include <ns16550.h>
#include <stdio.h>

#define BUFSIZE 256

int
vprintf(const char *format,
	va_list ap)
{
        char buffer[BUFSIZE];
	struct npr_printf_format fmt[16];
	struct npr_printf_arg args[16];
	int num_fmt;
	struct npr_printf_state st;
	int is_fini, out_size;

	num_fmt = npr_printf_build_format(fmt, 16, format, strlen(format));

	if ((num_fmt < 0) || (num_fmt >= 16)) {
		puts("too many format for printf");
		while (1);
	}
	npr_printf_build_varg(args, fmt, num_fmt, ap);

	npr_sprintf_start(&st);
	out_size = npr_sprintf(&st, buffer, BUFSIZE, fmt, num_fmt, args, &is_fini);

	if (! is_fini) {
		puts("printf error");
		return -1;
	}

	ns16550_write_text(buffer, out_size);

	return out_size;
}


int
puts(const char *str)
{
	char crlf[] = "\r\n";
	int len = strlen(str);

        ns16550_write(str, len);
        ns16550_write(crlf, 2);

	return len+2;
}
