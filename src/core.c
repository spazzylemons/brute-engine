#include "core.h"

void *Allocate(size_t size) {
    if (size == 0) {
        // Don't make empty allocations.
        size = 1;
    }
    return playdate->system->realloc(NULL, size);
}

void Deallocate(void *ptr) {
    // Don't free NULL.
    if (ptr != NULL) {
        playdate->system->realloc(ptr, 0);
    }
}
