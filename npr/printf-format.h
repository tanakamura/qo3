#ifndef NPR_PRINTF_FORMAT_H
#define NPR_PRINTF_FORMAT_H

#include "printf.h"
#include <stdarg.h>

/* returns number of dest
 * -1 : too many arguments
 * -2 : invalid format
 */
int npr_printf_build_format(struct npr_printf_format *dest,
			    int max_format,
			    const char *format,
			    int format_len);

void npr_printf_build_arg(struct npr_printf_arg *dest,
			  const struct npr_printf_format *fmt,
			  int format_len,
			  ...);
void npr_printf_build_varg(struct npr_printf_arg *dest,
			   const struct npr_printf_format *fmt,
			   int format_len,
			   va_list ap);

#endif
