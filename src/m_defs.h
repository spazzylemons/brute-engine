#ifndef BRUTE_M_DEFS_H
#define BRUTE_M_DEFS_H

#include "u_aabb.h"

#include <stdint.h>

typedef struct {
    // The first vertex of this wall.
    vector_t *v1;
    // The second vertex of this wall.
    vector_t *v2;
    // Precalculated v2 - v1.
    vector_t delta;
    // Precalculated normal vector of this wall.
    vector_t normal;
    // The sector this wall is a portal to, or NULL if solid.
    struct sector_t *portal;
} wall_t;

typedef struct sector_t {
    // The next sector in the multipurpose set.
    struct sector_t *next_seen;
    // The next sector in the multipurpose queue.
    struct sector_t *next_queue;
    // The bounding box of this sector.
    aabb_t bounds;
    // The number of walls in this sector.
    uint16_t num_walls;
    // The walls in this sector.
    wall_t *walls;
} sector_t;

typedef struct {
    // The vertices in this map.
    vector_t *vtxs;
    // The sectors in this map.
    sector_t *scts;
    // The walls in this map.
    wall_t *walls;
} map_t;

// Test if a point is inside a sector.
bool M_SectorContainsPoint(const sector_t *sector, const vector_t *point);

#endif
