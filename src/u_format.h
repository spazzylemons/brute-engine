#ifndef BRUTE_U_FORMAT_H
#define BRUTE_U_FORMAT_H

/**
 * String formatting.
 */

#include <stdarg.h>
#include <stddef.h>

void FormatString(char *buf, size_t n, const char *fmt, ...);

void FormatStringV(char *buf, size_t n, const char *fmt, va_list list);

#endif
