#ifndef BRUTE_U_FILE_H
#define BRUTE_U_FILE_H

#include <stddef.h>

/**
 * Wrappers over the Playdate file APIs for convenience. These functions
 * generally throw an error on failure.
 */

// The maximum file size that we will attempt to read. Any file larger than this
// will not be read, and an error will be thrown.
#define MAXFILESIZE (1 << 20)

// Read a file, and optionally get its size. The returned pointer should be
// freed with Deallocate.
void *U_FileRead(const char *path, size_t *size);

#endif
