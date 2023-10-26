#include "b_core.h"
#include "i_memory.h"
#include "u_error.h"
#include "u_list.h"

#ifdef DEBUG_ALLOCATOR

#define MAGIC_NUMBER 0xd09ef154 // dopefish lives!

// A block of allocated data.
typedef struct {
    // Connection to list.
    list_t list;
    // Magic number to validate block of data.
    uint32_t magic;
    // Line number where block was allocated.
    uint32_t line;
    // File name where block was allocated.
    const char *file;
    // Size of allocated memory.
    size_t size;
    // Memory allocation. Must be aligned properly.
    uint8_t __attribute__((aligned(16))) data[0];
} allocblock_t;

// All allocated blocks.
static EMPTY_LIST(allocations);

void *Allocate2(size_t size, const char *file, uint32_t line) {
    // Allocate block of memory.
    allocblock_t *block = playdate->system->realloc(NULL, size + sizeof(allocblock_t));
    // Check if allocation failed.
    if (block == NULL) {
        Error("%s:%u: Allocation of size %zu failed", file, line, size);
    }
    // Fill in data.
    U_ListInsert(&allocations, &block->list);
    block->magic = MAGIC_NUMBER;
    block->line = line;
    block->file = file;
    block->size = size;
    // Return a pointer to the data.
    return block->data;
}

void Deallocate2(void *ptr, const char *file, uint32_t line) {
    // Don't free NULL.
    if (ptr == NULL) {
        return;
    }
    // Convert to block pointer.
    allocblock_t *block = (allocblock_t *) (((char *) ptr) - __builtin_offsetof(allocblock_t, data));
    if (block->magic != MAGIC_NUMBER) {
        // Most of the time an invalid free will just crash. But if we can
        // catch some cases, why not?
        Error("%s:%u: Invalid free", file, line);
    }
    // Free data.
    U_ListRemove(&block->list);
    playdate->system->realloc(block, 0);
}

void CheckLeaks(void) {
    listiter_t iter;
    U_ListIterInit(&iter, &allocations);
    allocblock_t *block;

    while ((block = (allocblock_t *) U_ListIterNext(&iter)) != NULL) {
        playdate->system->logToConsole("%s:%u: %zu bytes leaked", block->file, block->line, block->size);
    }
}

#else

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

#endif
