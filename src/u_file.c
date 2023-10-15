#include "b_core.h"
#include "u_error.h"
#include "u_file.h"

void *U_FileRead(const char *path, size_t *size) {
    // Open the file.
    SDFile *file = playdate->file->open(path, kFileRead | kFileReadData);
    if (file == NULL) {
        Error("U_FileRead: File '%s' could not be opened", path);
    }
    // Get the size of the file.
    int sizeval;
    if (playdate->file->seek(file, 0, SEEK_END) ||
        (sizeval = playdate->file->tell(file)) < 0 ||
        playdate->file->seek(file, 0, SEEK_SET)) {
        Error("U_FileRead: Failed to get size of file '%s'", path);
    }
    // Check max file size.
    if (sizeval > MAXFILESIZE) {
        Error("U_FileRead: File '%s' is too large", path);
    }
    // Allocate the buffer.
    void *buffer = Allocate(sizeval);
    // Read into the buffer.
    if (playdate->file->read(file, buffer, sizeval) != sizeval) {
        Error("U_FileRead: Failed to read entire file '%s'", path);
    }
    // Close the file.
    playdate->file->close(file);
    // Write size if requested.
    if (size != NULL) {
        *size = sizeval;
    }
    // Return the buffer.
    return buffer;
}
