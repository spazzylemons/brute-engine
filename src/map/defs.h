#ifndef BRUTE_MAP_DEFS_H
#define BRUTE_MAP_DEFS_H

#include "../b_aabb.h"

typedef struct {
    // The first vertex of this wall.
    vertex_t *v1;
    // The second vertex of this wall.
    vertex_t *v2;
    // Precalculated v2 - v1.
    vertex_t delta;
    // Precalculated normal vector of this wall.
    vertex_t normal;
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
    vertex_t *vtxs;
    // The sectors in this map.
    sector_t *scts;
    // The walls in this map.
    wall_t *walls;
} map_t;

// A structure for iterating over sectors without duplicates.
// Note that this uses fields within the sectors, so it should not be used
// recursively on the same map.
typedef struct {
    // The first sector iterated over.
    sector_t *root;
    // The set of used sectors.
    sector_t *seen;
    // The head of the queue.
    sector_t *head;
    // The tail of the queue.
    sector_t *tail;
} sectoriter_t;

// Initialize a sector iterator.
void M_SectorIterNew(sectoriter_t *iter, sector_t *first);

// Push a sector to a sector iterator's queue.
void M_SectorIterPush(sectoriter_t *iter, sector_t *sector);

// Pop a sector off of the sector iterator's queue. Returns NULL if empty.
sector_t *M_SectorIterPop(sectoriter_t *iter);

// Clean up links in all iterated sectors.
void M_SectorIterCleanup(sectoriter_t *iter);

// Test if a point is inside a sector.
bool M_SectorContainsPoint(const sector_t *sector, const vertex_t *point);

#endif
