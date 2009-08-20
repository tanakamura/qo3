#ifndef NPR_PRINTF_H
#define NPR_PRINTF_H

#ifdef __cplusplus
extern "C" {
#endif

enum npr_printf_format_type {
	NPR_PRINTF_DIGIT,	/* %d */
	NPR_PRINTF_LDIGIT,	/* %ld */
	NPR_PRINTF_LLDIGIT,	/* %lld */
	NPR_PRINTF_UDIGIT,	/* %u */
	NPR_PRINTF_LUDIGIT,	/* %lu */
	NPR_PRINTF_LLUDIGIT,	/* %llu */
	NPR_PRINTF_hex,		/* %x */
	NPR_PRINTF_lhex,	/* %lx */
	NPR_PRINTF_llhex,	/* %llx */
	NPR_PRINTF_HEX,		/* %X */
	NPR_PRINTF_LHEX,	/* %lX */
	NPR_PRINTF_LLHEX,	/* %llX */
	NPR_PRINTF_POINTER,	/* %p */
	NPR_PRINTF_STR,		/* %s */
	NPR_PRINTF_CHAR,	/* %c */
	NPR_PRINTF_ORDINARY,	/* "abc" */
	NPR_PRINTF_ORDINARY_CHAR /* "a" */
};

struct npr_printf_arg {
	const char *p;
	union {
		long long sll;
		unsigned long long ull;
		long sl;
		unsigned long ul;
		signed int si;
		unsigned int ui;
		void *p;
		char c;
	} u;
};

#define NPR_PRINTF_ARG_INT(i) {0,.u.si=i},
#define NPR_PRINTF_ARG_LONG(i) {0,.u.sl=i},
#define NPR_PRINTF_ARG_LLONG(i) {0,.u.sll=i},
#define NPR_PRINTF_ARG_UINT(i) {0,.u.ui=i},
#define NPR_PRINTF_ARG_ULONG(i) {0,.u.ul=i},
#define NPR_PRINTF_ARG_ULLONG(i) {0,.u.ull=i},
#define NPR_PRINTF_ARG_POINTER(x) {0,.u.p=x},
#define NPR_PRINTF_ARG_CHAR(x) {0,.u.c=x},

#define NPR_PRINTF_ARG_STR_C(s) {s, .u.si=sizeof(s)-1},
#define NPR_PRINTF_ARG_STR(s,l) {s, .u.si=l},

#define NPR_PRINTF_SET_ARG_INT(s,n) do {s.p=0; s.i=(n);} while(0)
#define NPR_PRINTF_SET_ARG_STR_C(s,t) do {s.p=t; s.i=sizeof(t)-1;} while(0)
#define NPR_PRINTF_SET_ARG_STR(s,l) do {s.p=t; s.i=l;} while(0)

struct npr_printf_format {
	union {
		const char *ordinary;
		int wid;
	} u;
	int zero_fill;
	enum npr_printf_format_type type;
};

#define NPR_FORMAT_DIGIT(w,z) {{(const char*)w}, z, NPR_PRINTF_DIGIT},
#define NPR_FORMAT_hex(w,z) {{(const char*)w}, z, NPR_PRINTF_hex},
#define NPR_FORMAT_HEX(w,z) {{(const char*)w}, z, NPR_PRINTF_HEX},
#define NPR_FORMAT_UDIGIT(w,z) {{(const char*)w}, z, NPR_PRINTF_UDIGIT},

#define NPR_FORMAT_LDIGIT(w,z) {{(const char*)w}, z, NPR_PRINTF_LDIGIT},
#define NPR_FORMAT_lhex(w,z) {{(const char*)w}, z, NPR_PRINTF_lhex},
#define NPR_FORMAT_LHEX(w,z) {{(const char*)w}, z, NPR_PRINTF_LHEX},
#define NPR_FORMAT_LUDIGIT(w,z) {{(const char*)w}, z, NPR_PRINTF_LUDIGIT},

#define NPR_FORMAT_LLDIGIT(w,z) {{(const char*)w}, z, NPR_PRINTF_LLDIGIT},
#define NPR_FORMAT_llhex(w,z) {{(const char*)w}, z, NPR_PRINTF_llhex},
#define NPR_FORMAT_LLHEX(w,z) {{(const char*)w}, z, NPR_PRINTF_LLHEX},
#define NPR_FORMAT_LLUDIGIT(w,z) {{(const char*)w}, z, NPR_PRINTF_LLUDIGIT},

#define NPR_FORMAT_POINTER(w) {{(const char*)w}, 0, NPR_PRINTF_POINTER},
#define NPR_FORMAT_STR(w) {{(const char*)w}, 0, NPR_PRINTF_STR},
#define NPR_FORMAT_ORDINARY_C(s) {{s}, sizeof(s)-1, NPR_PRINTF_ORDINARY},
#define NPR_FORMAT_ORDINARY(s,l) {{s}, l, NPR_PRINTF_ORDINARY},
#define NPR_FORMAT_ORDINARY_CHAR(c) {{0}, c, NPR_PRINTF_ORDINARY_CHAR},

struct npr_printf_state
{
	int state;
	int format_index;
	int arg_index;
	int dump_digit_size;
	int char_index;
	unsigned char num_digit;
	unsigned char sign;
	char dump_digit_buf[16];
};

void npr_sprintf_start(struct npr_printf_state *st);

int npr_sprintf(struct npr_printf_state *st,
		char *dst,
		int dstlen,
		const struct npr_printf_format *formats,
		int nformat,
		const struct npr_printf_arg *args,
		int *is_fini);

#ifdef __cplusplus
}
#endif

#endif
