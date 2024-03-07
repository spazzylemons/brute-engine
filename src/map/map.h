#ifndef BRUTE_M_MAP_H
#define BRUTE_M_MAP_H

#include "actor/actor.h"
#include "map/defs.h"

#define OBJ_RADIUS 20.0f

#define MAX_STAIR_HEIGHT 24.0f

#define MAP_CLASS "brute.classes.map"

void M_MoveAndSlide(actor_t *actor);

// Test if a point is in front of a wall.
bool M_PointInFrontOfWall(const wall_t *wall, const vector_t *point);

// Test if a point is inside a sector.
bool M_SectorContainsPoint(const sector_t *sector, const vector_t *point);

// Test if a circle is within a sector.
bool M_SectorContainsCircle(const sector_t *sector, const vector_t *point, float radius);

void register_map_class(void);

#endif
