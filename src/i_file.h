#ifndef BRUTE_I_FILE_H
#define BRUTE_I_FILE_H

/**
 * Lower-level file management functions.
 */

#include <stdint.h>

// Opaque pointer to file object.
typedef struct file_s file_t;

typedef enum {
    OPEN_RD,
    OPEN_WR,
} openmode_t;

// Seek modes.
#ifndef SEEK_SET
#define SEEK_SET 0
#define SEEK_CUR 1
#define SEEK_END 2
#endif

// Open a file. Returns NULL on failure.
file_t *I_FileOpen(const char *path, openmode_t mode);

// Close a file.
void I_FileClose(file_t *file);

// Seek to a position in a file.
void I_FileSeek(file_t *file, int32_t amt, int whence);

// Check where a file's position is.
uint32_t I_FileTell(file_t *file);

// Read a file. Returns the number of bytes read.
uint32_t I_FileRead(file_t *file, void *buffer, uint32_t size);

#endif
