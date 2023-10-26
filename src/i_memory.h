#ifndef BRUTE_I_MEMORY_H
#define BRUTE_I_MEMORY_H

/**
 * Low-level memory allocation.
 */

#include <stddef.h>

void *I_Malloc(size_t size);

void I_Free(void *ptr);

#endif
