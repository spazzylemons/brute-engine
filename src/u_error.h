#ifndef BRUTE_U_ERROR_H
#define BRUTE_U_ERROR_H

#include <stdnoreturn.h>

// Raise an fatal error. Can only be called if CatchError is called higher up
// in the stack.
noreturn void Error(const char *fmt, ...);

// If this returns a number, then an error has occurred.
int CatchError(void);

// Display the error. Should not be called if no error has occurred.
void DisplayError(void);

#endif
