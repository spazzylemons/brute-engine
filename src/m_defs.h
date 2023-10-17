#ifndef BRUTE_M_DEFS_H
#define BRUTE_M_DEFS_H

#include "u_aabb.h"

#include <stddef.h>
#include <stdint.h>

// A wall texture, or "patch".
typedef struct patch_s {
    // The next patch in the list.
    struct patch_s *next;
    // The width of the texture.
    uint16_t width;
    // The height of the texture.
    uint16_t height;
    // The number of bytes per column.
    uint16_t stride;
    // The texture data, stored in columns.
    uint8_t data[0];
} patch_t;

// A floor/ceiling texture, or "flat". All flats are 64x64 pixels in size.
typedef struct flat_s {
    // The next flat in the list.
    struct flat_s *next;
    // The texture data, stored in rows.
    uint8_t data[64 * 8];
} flat_t;

typedef struct {
    // The first vertex of this wall.
    vector_t *v1;
    // The second vertex of this wall.
    vector_t *v2;
    // Precalculated v2 - v1.
    vector_t delta;
    // Precalculated normal vector of this wall.
    vector_t normal;
    // Precalculated length of wall.
    float length;
    // Texture X offset.
    float xoffset;
    // The sector this wall is a portal to, or NULL if solid.
    struct sector_s *portal;
    // The patch used for this wall.
    patch_t *patch;
} wall_t;

typedef struct sector_s {
    // The next sector in the multipurpose set.
    struct sector_s *next_seen;
    // The next sector in the multipurpose queue.
    struct sector_s *next_queue;
    // The bounding box of this sector.
    aabb_t bounds;
    // The number of walls in this sector.
    uint16_t num_walls;
    // The walls in this sector.
    wall_t *walls;
    // The floor height.
    int16_t floor;
    // The ceiling height.
    int16_t ceiling;
    // The flat used for this sector's floor.
    flat_t *floorflat;
} sector_t;

typedef struct {
    // The vertices in this map.
    vector_t *vtxs;
    // The number of vertices in this map.
    size_t numvtxs;
    // The sectors in this map.
    sector_t *scts;
    // The number of sectors in this map.
    size_t numscts;
    // The walls in this map.
    wall_t *walls;
    // The number of walls in this map.
    size_t numwalls;
    // The linked list of patches in this map.
    patch_t *patches;
    // The flat used in this map.
    flat_t *flats;
} map_t;

// Test if a point is inside a sector.
bool M_SectorContainsPoint(const sector_t *sector, const vector_t *point);

#endif
