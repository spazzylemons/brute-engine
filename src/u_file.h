#ifndef BRUTE_U_FILE_H
#define BRUTE_U_FILE_H

#include <stddef.h>

/**
 * Wrappers over the Playdate file APIs for convenience. These functions
 * generally throw an error on failure.
 */

// Read a file, and optionally get its size. The returned pointer should be
// freed with Deallocate.
void *U_FileRead(const char *path, size_t *size);

#endif
