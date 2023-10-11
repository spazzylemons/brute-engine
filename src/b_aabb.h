#ifndef BRUTE_B_AABB_H
#define BRUTE_B_AABB_H

/**
 * Axis-aligned bounding box for various purposes.
 */

#include "b_types.h"

#include <stdbool.h>

typedef struct {
    vertex_t min;
    vertex_t max;
} aabb_t;

bool B_AABBOverlap(const aabb_t *a, const aabb_t *b);

#endif
