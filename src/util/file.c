#include "system.h"

void *read_file(const char *path, size_t *size) {
    // Open the file.
    SDFile *file = playdate->file->open(path, kFileRead | kFileReadData);
    if (file == NULL)
        playdate->system->error("File '%s' not found", path);

    // How big is the file? Only ask *after* we opened it to avoid TOCTOU
    FileStat stat;
    if (playdate->file->stat(path, &stat))
        playdate->system->error("Failed to stat '%s'", path);

    // Create a buffer.
    void *buffer = playdate->system->realloc(NULL, stat.size);
    if (playdate->file->read(file, buffer, stat.size) != (int) stat.size)
        playdate->system->error("Failed to read entire file '%s'", path);

    playdate->file->close(file);

    if (size != NULL)
        *size = stat.size;
    return buffer;
}
