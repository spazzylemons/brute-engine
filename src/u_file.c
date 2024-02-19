#include "i_file.h"
#include "i_system.h"
#include "u_file.h"
#include "z_memory.h"

void *U_FileRead(const char *path, size_t *size) {
    // Open the file.
    file_t *file = I_FileOpen(path, OPEN_RD);
    if (file == NULL) {
        I_Error("U_FileRead: File '%s' could not be opened", path);
    }
    // Get the size of the file.
    I_FileSeek(file, 0, SEEK_END);
    uint32_t sizeval = I_FileTell(file);
    I_FileSeek(file, 0, SEEK_SET);
    // Check max file size.
    if (sizeval > MAXFILESIZE) {
        I_Error("U_FileRead: File '%s' is too large", path);
    }
    // Allocate the buffer.
    void *buffer = Allocate(sizeval);
    // Read into the buffer.
    if (I_FileRead(file, buffer, sizeval) != sizeval) {
        I_Error("U_FileRead: Failed to read entire file '%s'", path);
    }
    // Close the file.
    I_FileClose(file);
    // Write size if requested.
    if (size != NULL) {
        *size = sizeval;
    }
    // Return the buffer.
    return buffer;
}
