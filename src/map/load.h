#ifndef BRUTE_M_LOAD_H
#define BRUTE_M_LOAD_H

#include "map/defs.h"

// Loads a map from /maps/$name.
map_t *map_load(const char *name);

// Frees a map.
void map_free(map_t *map);

#endif
