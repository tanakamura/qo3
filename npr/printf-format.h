#ifndef NPR_PRINTF_FORMAT_H
#define NPR_PRINTF_FORMAT_H

#include "printf.h"
#include <stdarg.h>

enum npr_printf_build_format_error_code {
	NPR_PRINTF_BUILD_FORMAT_TOO_MANY_FORMATS,
	NPR_PRINTF_BUILD_FORMAT_INVALID_FORMAT
};

struct npr_printf_build_format_error {
	enum npr_printf_build_format_error_code code;
	int idx;
};

/* returns number of dest
 * negative if failed.
 */
int npr_printf_build_format(struct npr_printf_format *dest,
			    int max_format,
			    const char *format,
			    int format_len,
			    struct npr_printf_build_format_error *er);

void npr_printf_build_arg(struct npr_printf_arg *dest,
			  const struct npr_printf_format *fmt,
			  int format_len,
			  ...);
void npr_printf_build_varg(struct npr_printf_arg *dest,
			   const struct npr_printf_format *fmt,
			   int format_len,
			   va_list ap);

#endif
