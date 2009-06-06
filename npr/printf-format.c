#include "printf-format.h"
#include <ctype.h>
#include <stdarg.h>
#include <string.h>

#define TOO_MANY_FORMAT -1
#define INVALID_FORMAT -2

int
npr_printf_build_format(struct npr_printf_format *f,
			int max_format,
			const char *format,
			int format_len)
{
	char c;
	int i = 0;
	int j = 0;
	int zero_fill, wid, d;
	int longlongflag;

#define CHECK_I(ret) if (i >= format_len) { return (ret); }
#define CHECK_J(ret) if (j >= max_format) { return (ret); }

format_start:
	CHECK_I(j);

	c = format[i];
	if (c == '%') {
		i++;
		goto parse_fmt_number;
	}
	CHECK_J(TOO_MANY_FORMAT);

	f[j].u.ordinary = format + i;
	f[j].zero_fill = 0;
	f[j].type = NPR_PRINTF_ORDINARY;

ord_str:
	CHECK_I(j+1);
	c = format[i];
	i++;
	if (c == '%') {
		/* fini ord */
		j++;
		goto parse_fmt_number;
	}
	f[j].zero_fill++;
	goto ord_str;

parse_fmt_number:
	CHECK_I(INVALID_FORMAT);

	c = format[i];
	zero_fill = 0;
	if (c == '0') {
		zero_fill = 1;
		i++;
	}
	wid = 0;
	while (1) {
		CHECK_I(INVALID_FORMAT);
		c = format[i];

		if (!isdigit(c))
			break;

		i++;
		d = c-'0';
		wid = wid*10 + d;
	}

	longlongflag = 0;
	c = format[i];

	if (c == 'l') {
		longlongflag++;
		i++;

		CHECK_I(INVALID_FORMAT);
		c = format[i];

		if (c == 'l') {
			longlongflag++;
			i++;
		}
	}


	CHECK_I(INVALID_FORMAT);
	CHECK_J(TOO_MANY_FORMAT);

	c = format[i];

	switch (longlongflag) {
	case 0:
		switch (c) {
		case 'd':
			f[j].type = NPR_PRINTF_DIGIT;
			break;

		case 'u':
			f[j].type = NPR_PRINTF_UDIGIT;
			break;

		case 'x':
			f[j].type = NPR_PRINTF_HEX;
			break;

		case 's':
			f[j].type = NPR_PRINTF_STR;
			break;

		case 'p':
			f[j].type = NPR_PRINTF_POINTER;

		default:
			return INVALID_FORMAT;
		}

		break;

	case 1:
		switch (c) {
		case 'd':
			f[j].type = NPR_PRINTF_LDIGIT;
			break;

		case 'u':
			f[j].type = NPR_PRINTF_LUDIGIT;
			break;

		case 'x':
			f[j].type = NPR_PRINTF_LHEX;
			break;

		default:
			return INVALID_FORMAT;
		}
		break;

	case 2:
		switch (c) {
		case 'd':
			f[j].type = NPR_PRINTF_LLDIGIT;
			break;

		case 'u':
			f[j].type = NPR_PRINTF_LLUDIGIT;
			break;

		case 'x':
			f[j].type = NPR_PRINTF_LLHEX;
			break;

		default:
			return INVALID_FORMAT;
		}
		break;
	}
	f[j].u.wid = wid;
	f[j].zero_fill = zero_fill;

	i++;
	j++;

	goto format_start;
}

void
npr_printf_build_varg(struct npr_printf_arg *dest,
		      const struct npr_printf_format *fmt,
		      int n,
		      va_list ap)
{
	int i, d;
	long l;
	long long ll;
	int j = 0;
	char *s;
	void *p;

	for (i=0; i<n; i++) {
		switch (fmt[i].type) {
		case NPR_PRINTF_DIGIT:
		case NPR_PRINTF_UDIGIT:
		case NPR_PRINTF_HEX:
			d = va_arg(ap, int);
			dest[j].p = 0;
			dest[j].u.si = d;
			j++;
			break;

		case NPR_PRINTF_LDIGIT:
		case NPR_PRINTF_LUDIGIT:
		case NPR_PRINTF_LHEX:
			l = va_arg(ap, long);
			dest[j].p = 0;
			dest[j].u.sl = l;
			j++;
			break;

		case NPR_PRINTF_LLDIGIT:
		case NPR_PRINTF_LLUDIGIT:
		case NPR_PRINTF_LLHEX:
			ll = va_arg(ap, long long);
			dest[j].p = 0;
			dest[j].u.sll = ll;
			j++;
			break;

		case NPR_PRINTF_POINTER:
			p = va_arg(ap, void *);
			dest[j].p = 0;
			dest[j].u.p = p;
			j++;
			break;

		case NPR_PRINTF_STR:
			s = va_arg(ap, char*);
			dest[j].p = s;
			dest[j].u.si = strlen(s);
			j++;
			break;

		case NPR_PRINTF_ORDINARY:
		case NPR_PRINTF_ORDINARY_CHAR:
			break;
		}
	}
}

void
npr_printf_build_arg(struct npr_printf_arg *dest,
		     const struct npr_printf_format *fmt,
		     int n,
		     ...)
{
	va_list ap;
	va_start(ap, n);
	npr_printf_build_varg(dest, fmt, n, ap);
	va_end(ap);
}
