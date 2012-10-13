#include "printf-format.h"
#include <ctype.h>
#include <stdarg.h>
#include <string.h>
#include <stdio.h>

int
npr_printf_build_format(struct npr_printf_format *f,
			int max_format,
			const char *format,
			int format_len,
			struct npr_printf_build_format_error *error)
{
	char c;
	int i = 0;		/* index of format charctors */
	int j = 0;		/* index of dest format */
	int zero_fill, wid, d;
	int longlongflag;
	int halfflag;

#define CHECK_I(ret) if (i >= format_len) { return (ret); }
#define CHECK_J(ret) if (j >= max_format) { return (ret); }
#define CHECK_I_ERROR(c) if (i >= format_len) { error->code=c; error->idx=i; return -1; }
#define CHECK_J_ERROR(c) if (j >= max_format) { error->code=c; return -1; }

format_start:
	CHECK_I(j);

	c = format[i];
	if (c == '%') {
		i++;
		goto parse_fmt_number;
	}
	CHECK_J_ERROR(NPR_PRINTF_BUILD_FORMAT_TOO_MANY_FORMATS);

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
	CHECK_I_ERROR(NPR_PRINTF_BUILD_FORMAT_INVALID_FORMAT);

	c = format[i];
	zero_fill = 0;
	if (c == '0') {
		zero_fill = 1;
		i++;
	}
	wid = 0;
	while (1) {
		CHECK_I_ERROR(NPR_PRINTF_BUILD_FORMAT_INVALID_FORMAT);
		c = format[i];

		if (!isdigit(c))
			break;

		i++;
		d = c-'0';
		wid = wid*10 + d;
	}

	longlongflag = 0;
	halfflag=0;
	c = format[i];

	if (c == '.' || c == '-') {
		/* fixme */
		i++;
		goto parse_fmt_number;
	}

	if (c == 'l') {
		longlongflag++;
		i++;

		CHECK_I_ERROR(NPR_PRINTF_BUILD_FORMAT_INVALID_FORMAT);
		c = format[i];

		if (c == 'l') {
			longlongflag++;
			i++;
		}
	}
	if (c == 'h') {
		halfflag++;
		i++;

		CHECK_I_ERROR(NPR_PRINTF_BUILD_FORMAT_INVALID_FORMAT);
		c = format[i];

		if (c == 'h') {
			halfflag++;
			i++;
		}
		/* fixme : halfflag */
	}

	CHECK_I_ERROR(NPR_PRINTF_BUILD_FORMAT_INVALID_FORMAT);
	CHECK_J_ERROR(NPR_PRINTF_BUILD_FORMAT_TOO_MANY_FORMATS);

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
			f[j].type = NPR_PRINTF_hex;
			break;

		case 'X':
			f[j].type = NPR_PRINTF_HEX;
			break;

		case 's':
			f[j].type = NPR_PRINTF_STR;
			break;

		case 'p':
			f[j].type = NPR_PRINTF_POINTER;
			break;

		case 'c':
			f[j].type = NPR_PRINTF_CHAR;
			break;

		default:
			error->code = NPR_PRINTF_BUILD_FORMAT_INVALID_FORMAT;
			error->idx = i;
			return -1;
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
			f[j].type = NPR_PRINTF_lhex;
			break;

		case 'X':
			f[j].type = NPR_PRINTF_LHEX;
			break;

		default:
			error->code = NPR_PRINTF_BUILD_FORMAT_INVALID_FORMAT;
			error->idx = i;
			return -1;
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
			f[j].type = NPR_PRINTF_llhex;
			break;

		case 'X':
			f[j].type = NPR_PRINTF_LLHEX;
			break;


		default:
			error->code = NPR_PRINTF_BUILD_FORMAT_INVALID_FORMAT;
			error->idx = i;
			return -1;
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
	char c;
	void *p;

	for (i=0; i<n; i++) {
		switch (fmt[i].type) {
		case NPR_PRINTF_DIGIT:
		case NPR_PRINTF_UDIGIT:
		case NPR_PRINTF_HEX:
		case NPR_PRINTF_hex:
			d = va_arg(ap, int);
			dest[j].p = 0;
			dest[j].u.si = d;
			j++;
			break;

		case NPR_PRINTF_LDIGIT:
		case NPR_PRINTF_LUDIGIT:
		case NPR_PRINTF_LHEX:
		case NPR_PRINTF_lhex:
			l = va_arg(ap, long);
			dest[j].p = 0;
			dest[j].u.sl = l;
			j++;
			break;

		case NPR_PRINTF_LLDIGIT:
		case NPR_PRINTF_LLUDIGIT:
		case NPR_PRINTF_LLHEX:
		case NPR_PRINTF_llhex:
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

		case NPR_PRINTF_CHAR:
			c = va_arg(ap, int);
			dest[j].p = 0;
			dest[j].u.c = c;
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
