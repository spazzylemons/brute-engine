#include "i_system.h"
#include "u_format.h"
#include "y_log.h"

#include <string.h>

void Y_Log(const char *fmt, ...) {
    va_list list;
    va_start(list, fmt);
    const char *message;
    if (!strcmp(fmt, "%s")) {
        // Special case: if format string just prints another string,
        // don't use the internal buffer.
        message = va_arg(list, const char *);
    } else {
        // Otherwise, use an internal buffer to format the content to.
        static char logbuffer[512];
        FormatStringV(logbuffer, sizeof(logbuffer), fmt, list);
        message = logbuffer;
    }
    va_end(list);
    // Use system log method.
    I_Log(message);
}
