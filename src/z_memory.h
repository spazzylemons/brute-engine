#ifndef BRUTE_Z_MEMORY_H
#define BRUTE_Z_MEMORY_H

/**
 * Memory allocation functions. Wraps the low-level allocation functions with
 * optional memory leak tracing.
 */

#include <stdint.h>

// If this is defined, we use a debugging allocator. This allows tracking
// memory leaks, and a chance to detect invalid frees.
#define DEBUG_ALLOCATOR

// Debug versions of memory allocation functions are macros to pass file name
// and line number.
#ifdef DEBUG_ALLOCATOR

// Allocate memory. Size can be 0.
#define Allocate(_size_) Allocate2(_size_, __FILE__, __LINE__)

// Free memory. Freeing NULL does nothing.
#define Deallocate(_ptr_) Deallocate2(_ptr_, __FILE__, __LINE__)

// Log the size and allocation location for any memory still allocated.
void CheckLeaks(void);

// Debug allocate function.
void *Allocate2(size_t size, const char *file, uint32_t line);

// Debug deallocate function.
void Deallocate2(void *ptr, const char *file, uint32_t line);

#else
// Allocate memory. Size can be 0.
void *Allocate(size_t size);

// Free memory. Freeing NULL does nothing.
void Deallocate(void *ptr);

#endif

#endif
