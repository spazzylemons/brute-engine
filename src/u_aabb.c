#include "u_aabb.h"

bool AABBOverlap(const aabb_t *a, const aabb_t *b) {
    return
        a->min.x < b->max.x && a->max.x > b->min.x &&
        a->min.y < b->max.y && a->max.y > b->min.y;
}

void AABBExpandToFit(aabb_t *aabb, const vector_t *point) {
    if (point->x < aabb->min.x) {
        aabb->min.x = point->x;
    }
    if (point->x > aabb->max.x) {
        aabb->max.x = point->x;
    }
    if (point->y < aabb->min.y) {
        aabb->min.y = point->y;
    }
    if (point->y > aabb->max.y) {
        aabb->max.y = point->y;
    }
}
