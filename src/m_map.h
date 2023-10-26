#ifndef BRUTE_M_MAP_H
#define BRUTE_M_MAP_H

#include "m_defs.h"

#define OBJ_RADIUS 20.0f

#define MAX_STAIR_HEIGHT 24.0f

sector_t *M_MoveAndSlide(
    sector_t *sector,
    vector_t *pos,
    float zpos,
    vector_t *delta
);

// Test if a point is inside a sector.
bool M_SectorContainsPoint(const sector_t *sector, const vector_t *point);

// Test if a circle is within a sector.
bool M_SectorContainsCircle(const sector_t *sector, const vector_t *point, float radius);

#endif
