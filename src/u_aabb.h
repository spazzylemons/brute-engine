#ifndef BRUTE_U_AABB_H
#define BRUTE_U_AABB_H

/**
 * Axis-aligned bounding box for various purposes.
 */

#include "u_vec.h"

#include <stdbool.h>

typedef struct {
    vector_t min;
    vector_t max;
} aabb_t;

bool B_AABBOverlap(const aabb_t *a, const aabb_t *b);

#endif