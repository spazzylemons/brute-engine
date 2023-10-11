#ifndef BRUTE_M_LOAD_H
#define BRUTE_M_LOAD_H

#include "m_defs.h"

// Loads a map from /maps/$name.
map_t *M_Load(const char *name);

// Frees a map.
void M_Free(map_t *map);

#endif
