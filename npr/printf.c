#include "printf.h"
#include <stdint.h>

#define MAX(a,b) ((a)>(b)?(a):(b))

void
npr_sprintf_start(struct npr_printf_state *st)
{
	st->char_index = 0;
	st->format_index = 0;
	st->arg_index = 0;
	st->state = 0;
}
static int
dump_str(char *dst,
	 int dst_cur,
	 int dst_size,
	 const char *str,
	 int str_len,
	 int *charno_p,
	 int *cont_next)
{
	int i;
	int charno = *charno_p;
	if (charno == -1) {
		charno = 0;
	}

	for (i=charno; i<str_len; i++) {
		int src_i = i;
		if (dst_cur >= dst_size) {
			*charno_p = i;

			*cont_next = 0;
			return dst_cur;
		}

		dst[dst_cur] = str[src_i];
		dst_cur++;
	}

	*charno_p = 0;
	*cont_next = 1;

	return dst_cur;
}

static int
fill_with(char *dst,
	  int dst_cur, int dstlen,
	  char fill, int num_fill, int *char_index, int *is_fini)
{
	int ci;
	for (ci = *char_index;
	     ci < num_fill;
	     ci++) {
		if (dst_cur>=dstlen) {
			*is_fini = 0;
			*char_index = ci;
			return dst_cur;
		}

		dst[dst_cur] = fill;
		dst_cur++;
	}

	*is_fini = 1;
	*char_index = 0;

	return dst_cur;
}

static const char n2c_table[] = {
	'0', '1', '2', '3', '4', '5', '6', '7', '8', '9',
	'a', 'b', 'c', 'd', 'e', 'f'
};

static int
output_integer(struct npr_printf_state *st,
	       char *out,
	       int dstcur,
	       int dstlen,
	       int wid, int lead_zero, signed long long v, int base,
	       int *is_fini)
{
	char *tmp;
	int i = st->num_digit;
	int sign;
	signed long long sv;
	unsigned long long uv;
	unsigned int is_signed = 0;
	tmp = st->dump_digit_buf;

	switch (st->state) {
	case 0:
		sign = 0;
		uv = v;
		sv = v;

		if (base == 10) {
			is_signed = 1;
		}

		if (is_signed && (v < 0)) {
			sign = 1;
			sv = -v;
		}

		if (v) {
			i=0;
			if (is_signed) {
				while (sv) {
					tmp[i] = n2c_table[sv%base];
					i++;
					sv/=base;
				}
			} else {
				while (uv) {
					tmp[i] = n2c_table[uv%base];
					i++;
					uv/=(unsigned int)base;
				}
			}
		} else {
			tmp[0] = '0';
			i = 1;
		}
		st->sign = sign;
		st->state = 1;
		st->num_digit = i;

		/* fall through */

	case 1:
		sign = st->sign;
		if (lead_zero) {
			if (sign) {
				if (dstcur==dstlen) {
					*is_fini = 0;
					return dstcur;
				}

				out[dstcur] = '-';
				dstcur++;
			}
		} else {
			int num_space = wid-(i+sign);
			dstcur = fill_with(out, dstcur, dstlen,
					   ' ', num_space, &st->char_index, is_fini);
			if (*is_fini == 0) {
				return dstcur;
			}

		}
		st->state = 2;

		/* fall */

	case 2:
		sign = st->sign;
		i = st->num_digit;
		if ( lead_zero ) {
			int num_zero = wid-(i+sign);
			dstcur = fill_with(out, dstcur, dstlen,
					   '0', num_zero, &st->char_index, is_fini);
			if (*is_fini == 0) {
				return dstcur;
			}
		} else {
			if (sign) {
				if (dstcur == dstlen) {
					*is_fini = 0;
					return dstcur;
				}

				out[dstcur] = '-';
				dstcur++;
			}
		}
		st->char_index = 0;
		st->state = 3;

	case 3:
		i = st->num_digit;

		while (i > 0) {
			if (dstcur == dstlen) {
				st->num_digit = i;
				*is_fini = 0;
				return dstcur;
			}

			out[dstcur] = tmp[i-1];
			i--;
			dstcur++;
		}

		*is_fini = 1;
		st->state = 0;
		break;
	}

	return dstcur;
}

int npr_sprintf(struct npr_printf_state *st,
		char *dst,
		int dstlen,
		const struct npr_printf_format *formats,
		int nformat,
		const struct npr_printf_arg *args,
		int *is_fini)
{
	char *d = dst;
	int di=0;
	int fi, ai = st->arg_index;
	*is_fini = 0;

	for (fi = st->format_index;
	     fi < nformat;
	     fi++) {
		const struct npr_printf_format *f = &formats[fi];
		const struct npr_printf_arg *a = &args[ai];

		int wid = f->u.wid;

		switch(f->type) {
		case NPR_PRINTF_STR:
		{
			const char *s = a->p;
			int len = a->u.si;

			int outlen = MAX( len, wid );
			int d = outlen-len;

			switch (st->state) {
			case 0:
				di = fill_with(dst, di, dstlen,
					       ' ', d, &st->char_index, is_fini);
				if (*is_fini == 0)
					goto quit;

				st->state = 1;

				/* fa */
			case 1:
				di = dump_str(dst, di, dstlen,
					      s, len, &st->char_index, is_fini);
				if (*is_fini == 0)
					goto quit;

				st->state = 0;
				break;
			}
		}
		ai++;
		break;


#define CASE_DIGIT(casetag,type,uniontag,casttype,base)			\
		case casetag:						\
		{							\
			type v = a->u.uniontag;				\
			di = output_integer(st, d, di,			\
					    dstlen,			\
					    wid, f->zero_fill,		\
					    (casttype)v, base, is_fini); \
			if (*is_fini == 0)				\
				goto quit;				\
		}							\
		ai++;							\
		break;

		CASE_DIGIT(NPR_PRINTF_DIGIT, int, si, signed long long, 10);
		CASE_DIGIT(NPR_PRINTF_LDIGIT, long, sl, signed long long, 10);
		CASE_DIGIT(NPR_PRINTF_LLDIGIT, long long, sll, signed long long, 10);
		CASE_DIGIT(NPR_PRINTF_UDIGIT, unsigned int, ui, unsigned long long, 10);
		CASE_DIGIT(NPR_PRINTF_LUDIGIT, unsigned long, ul, unsigned long long, 10);
		CASE_DIGIT(NPR_PRINTF_LLUDIGIT, unsigned long long, ull, unsigned long long, 10);

		CASE_DIGIT(NPR_PRINTF_HEX, unsigned int, ui, unsigned long long, 16);
		CASE_DIGIT(NPR_PRINTF_LHEX, unsigned long, ul, unsigned long long, 16);
		CASE_DIGIT(NPR_PRINTF_LLHEX, unsigned long long, ull, unsigned long long, 16);
		CASE_DIGIT(NPR_PRINTF_POINTER, void *, p, uintptr_t, 16);

		case NPR_PRINTF_ORDINARY:
		{
			const char *s = f->u.ordinary;
			int len = f->zero_fill;

			di = dump_str(dst, di, dstlen,
				      s, len, &st->char_index, is_fini);

			if (*is_fini == 0)
				goto quit;
		}
		break;

		case NPR_PRINTF_ORDINARY_CHAR:
		{
			if (di >= dstlen)
				goto quit;

			dst[di] = f->zero_fill;
			di++;
		}
		break;

		}
	}
	*is_fini = 1;

quit:
	st->format_index = fi;
	st->arg_index = ai;

	return di;
}


