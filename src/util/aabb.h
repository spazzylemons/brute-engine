#ifndef BRUTE_U_AABB_H
#define BRUTE_U_AABB_H

/**
 * Axis-aligned bounding box for various purposes.
 */

#include "util/vec.h"

#include <stdbool.h>

typedef struct {
    vector_t min;
    vector_t max;
} aabb_t;

// Return true if the two bounding boxes overlap.
bool aabb_overlap(const aabb_t *a, const aabb_t *b);

// Expand the dimensions of this AABB to fit the given point.
void aabb_expand(aabb_t *aabb, const vector_t *point);

#endif
