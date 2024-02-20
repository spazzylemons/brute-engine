#include "i_system.h"
#include "u_list.h"
#include "y_log.h"
#include "z_memory.h"

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
        I_Error("%s:%u: Allocation of size %u failed", file, line, size);
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

void *Reallocate2(void *ptr, size_t size, const char *file, uint32_t line) {
    // If NULL, use regular allocate.
    if (ptr == NULL) {
        return Allocate2(size, file, line);
    }
    // Convert to block pointer.
    allocblock_t *block = (allocblock_t *) (((char *) ptr) - __builtin_offsetof(allocblock_t, data));
    if (block->magic != MAGIC_NUMBER) {
        // Most of the time an invalid realloc will just crash. But if we can
        // catch some cases, why not?
        I_Error("%s:%u: Invalid realloc", file, line);
    }
    // Remove from list.
    U_ListRemove(&block->list);
    // Perform reallocation.
    block = playdate->system->realloc(block, size + sizeof(allocblock_t));
    if (block == NULL) {
        I_Error("%s:%u: Reallocation of size %u failed", file, line, size);
    }
    // Update information.
    block->line = line;
    block->file = file;
    block->size = size;
    // Add back into list.
    U_ListInsert(&allocations, &block->list);
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
        I_Error("%s:%u: Invalid free", file, line);
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
        Y_Log("%s:%u: %u bytes leaked", block->file, block->line, block->size);
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
        I_Error("Allocation of size %u failed.", size);
    }
    return result;
}

void *Reallocate(void *ptr, size_t size) {
    void *result = playdate->system->realloc(ptr, size);
    if (result == NULL) {
        I_Error("Reallocation of size %u failed.", size);
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
