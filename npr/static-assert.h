#ifndef NPR_STATIC_ASSERT_H
#define NPR_STATIC_ASSERT_H

extern void npr_static_error(void) __attribute__((__error__("static assertion failed")));
#define NPR_STATIC_ASSERT(a) if (__builtin_constant_p(a) && !(a)) { npr_static_error(); }

#endif
