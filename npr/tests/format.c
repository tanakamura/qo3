#include "../printf-format.h"
#include "../printf-format.c"
#include "../printf.h"
#include "../printf.c"

int main()
{
	struct npr_printf_format formats[8];
	struct npr_printf_arg args[8];
	char buf[32];
	struct npr_printf_state st;
	int done,l;
	char *p = "aaa";

	int r = npr_printf_build_format(formats, 8, "%c __ %c %08X %.2d", strlen("%c __ %c %0.8X %.2s"));

	npr_printf_build_arg(args, formats, r, 'c', 'b', 111, p);

	npr_sprintf_start(&st);
	l = npr_sprintf(&st, buf, 32, formats, r, args, &done);
	buf[l] = '\0';

	puts(buf);
}
