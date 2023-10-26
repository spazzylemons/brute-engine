#ifndef BRUTE_U_ERROR_H
#define BRUTE_U_ERROR_H

#include <stdint.h>
#include <stdnoreturn.h>

// Internal jump buffer.
extern intptr_t jumpbuffer[5];

// Raise an fatal error. Can only be called if CatchError is called higher up
// in the stack.
noreturn void Error(const char *fmt, ...);

// If this returns nonzero, then an error has occurred.
#define CatchError() __builtin_setjmp(jumpbuffer)

// Display the error. Should not be called if no error has occurred.
void DisplayError(void);

#endif
