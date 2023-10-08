#ifndef BRUTE_MAP_LOAD_H
#define BRUTE_MAP_LOAD_H

#include "defs.h"

// Loads a level from /maps/$name.
map_t *M_Load(const char *name);

void M_Free(map_t *map);

#endif
