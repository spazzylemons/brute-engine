#include "u_aabb.h"

bool B_AABBOverlap(const aabb_t *a, const aabb_t *b) {
    return
        a->min.x < b->max.x && a->max.x > b->min.x &&
        a->min.y < b->max.y && a->max.y > b->min.y;
}
