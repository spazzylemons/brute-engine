#include "b_core.h"
#include "u_error.h"

void *Allocate(size_t size) {
    if (size == 0) {
        // Don't make empty allocations.
        size = 1;
    }
    void *result = playdate->system->realloc(NULL, size);
    if (result == NULL) {
        Error("Allocation of size %u failed.", size);
    }
    return result;
}

void Deallocate(void *ptr) {
    // Don't free NULL.
    if (ptr != NULL) {
        playdate->system->realloc(ptr, 0);
    }
}
