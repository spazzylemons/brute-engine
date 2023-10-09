#ifndef BRUTE_MAP_DEFS_H
#define BRUTE_MAP_DEFS_H

#include "../b_types.h"

typedef struct {
    // The first vertex of this wall.
    vertex_t *v1;
    // The second vertex of this wall.
    vertex_t *v2;
    // The sector this wall is a portal to, or NULL if solid.
    struct sector_t *portal;
} wall_t;

typedef struct sector_t {
    // The next sector in the list of drawn sectors.
    struct sector_t *next_drawn;
    // The next sector in the stack of sectors to draw.
    struct sector_t *next_queued;
    // The number of walls in this sector.
    uint16_t num_walls;
    // The walls in this sector.
    wall_t *walls;
} sector_t;

typedef struct {
    vertex_t *vtxs;
    sector_t *scts;
    wall_t *walls;
} map_t;

#endif
