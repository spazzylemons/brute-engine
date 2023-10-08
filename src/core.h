#ifndef BRUTE_CORE_H
#define BRUTE_CORE_H

#include "pd_api.h"

extern PlaydateAPI *playdate;

// Allocate memory. Size can be 0.
void *Allocate(size_t size);

// Free memory. Freeing NULL does nothing.
void Deallocate(void *ptr);

#endif
