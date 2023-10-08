#ifndef BRUTE_MAP_DEFS_H
#define BRUTE_MAP_DEFS_H

#include <stdint.h>

// TODO move out of here.
typedef int32_t fixed_t;

// TODO move out of here.
#define FRACBITS 16

// TODO move out of here.
typedef struct {
    fixed_t x;
    fixed_t y;
} vertex_t;

typedef struct {
    vertex_t *v1;
    vertex_t *v2;
    struct sector_t *portal;
} wall_t;

typedef struct sector_t {
    uint16_t num_walls;
    wall_t *walls;
} sector_t;

typedef struct {
    vertex_t *vtxs;
    sector_t *scts;
    wall_t *walls;
} map_t;

#endif
