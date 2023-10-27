#include "u_format.h"

#include <stdbool.h>
#include <stdint.h>

// Macro for writing characters into a buffer.
#define WRITE_CHAR(_char_) if (buf != end) *buf++ = _char_; else goto quit;

static char *FormatUnsignedInt(char *buf, char *end, unsigned int value) {
    // 10 characters is all we need to store the largest 32-bit integer.
    char intbuf[10];
    int bufidx = 0;
    // Convert integer in reverse.
    do {
        intbuf[bufidx++] = '0' + (value % 10);
        value /= 10;
    } while (value != 0);
    // Print reversed buffer in reverse.
    while (bufidx > 0) {
        WRITE_CHAR(intbuf[--bufidx]);
    }
quit:
    return buf;
}

static char *FormatHex(char *buf, char *end, uintptr_t value, bool upper) {
    // Buffer is as big as needed to fit pointer values.
    char hexbuf[sizeof(uintptr_t) * 2];
    int bufidx = 0;
    // Choose character set.
    const char *hexdigits = upper ? "0123456789ABCDEF" : "0123456789abcdef";
    // Convert integer in reverse.
    do {
        hexbuf[bufidx++] = hexdigits[value & 15];
        value >>= 4;
    } while (value != 0);
    // Print reversed buffer in reverse.
    while (bufidx > 0) {
        WRITE_CHAR(hexbuf[--bufidx]);
    }
quit:
    return buf;
}

void FormatString(char *buf, size_t n, const char *fmt, ...) {
    va_list list;
    va_start(list, fmt);
    FormatStringV(buf, n, fmt, list);
    va_end(list);
}

void FormatStringV(char *buf, size_t n, const char *fmt, va_list list) {
    // Can't format into an empty buffer.
    if (n == 0) return;
    // Leave space for a null terminator even if we exhaust the buffer space.
    char *end = &buf[n - 1];
    char c;
    // While we have space and a character to print...
    while ((buf != end) && (c = *fmt++)) {
        if (c == '%') {
            // Check format string.
            // Modifiers not yet supported.
            c = *fmt++;
            // Avoid reading past the null terminator.
            if (c == '\0') break;
            // Switch based on type.
            switch (c) {
                case 'd':
                case 'i': {
                    int val = va_arg(list, int);
                    if (val < 0) {
                        WRITE_CHAR('-');
                        buf = FormatUnsignedInt(buf, end, -val);
                    } else {
                        buf = FormatUnsignedInt(buf, end, val);
                    }
                    break;
                }
                case 'u': {
                    unsigned int val = va_arg(list, unsigned int);
                    buf = FormatUnsignedInt(buf, end, val);
                    break;
                }
                case 'x': {
                    unsigned int val = va_arg(list, unsigned int);
                    buf = FormatHex(buf, end, val, false);
                    break;
                }
                case 'X': {
                    unsigned int val = va_arg(list, unsigned int);
                    buf = FormatHex(buf, end, val, true);
                    break;
                }
                case 'p': {
                    void *val = va_arg(list, void *);
                    if (val == NULL) {
                        WRITE_CHAR('(');
                        WRITE_CHAR('n');
                        WRITE_CHAR('i');
                        WRITE_CHAR('l');
                        WRITE_CHAR(')');
                    } else {
                        WRITE_CHAR('0');
                        WRITE_CHAR('x');
                        buf = FormatHex(buf, end, (uintptr_t) val, false);
                    }
                    break;
                }
                case 's': {
                    const char *str = va_arg(list, const char *);
                    if (str == NULL) {
                        WRITE_CHAR('(');
                        WRITE_CHAR('n');
                        WRITE_CHAR('i');
                        WRITE_CHAR('l');
                        WRITE_CHAR(')');
                    } else {
                        while ((c = *str++)) {
                            WRITE_CHAR(c);
                        }
                    }
                    break;
                }
                case '%': {
                    WRITE_CHAR('%');
                    break;
                }
                case 'c': {
                    c = va_arg(list, int);
                    WRITE_CHAR(c);
                    break;
                }
                default: {
                    WRITE_CHAR('%');
                    WRITE_CHAR(c);
                    break;
                }
            }
        } else {
            // No special formatting.
            WRITE_CHAR(c);
        }
    }
    // Terminate string.
quit:
    *buf = '\0';
}
