#ifndef BRUTE_I_SYSTEM_H
#define BRUTE_I_SYSTEM_H

/**
 * System interface functions.
 */

#include <pd_api.h>
#include <stdnoreturn.h>

extern PlaydateAPI *playdate;

// Log a string to the console, followed by a line feed.
void I_Log(const char *string);

// Close the game with a fatal error.
noreturn void I_Error(const char *fmt, ...);

#endif