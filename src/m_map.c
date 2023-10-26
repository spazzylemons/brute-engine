#include "m_iter.h"
#include "m_map.h"
#include "u_vec.h"

#include <math.h>

// Height of object when colliding with ceilings.
#define OBJECT_HEIGHT 56.0f

static float WallPointDist(const wall_t *wall, const vector_t *point) {
    return (wall->v2->y - wall->v1->y) * (point->x - wall->v1->x) -
        (wall->v2->x - wall->v1->x) * (point->y - wall->v1->y);
}

bool M_SectorContainsPoint(const sector_t *sector, const vector_t *point) {
    for (size_t i = 0; i < sector->num_walls; i++) {
        const wall_t *wall = &sector->walls[i];
        // Test if point lies in front of line.
        if (WallPointDist(wall, point) < 0.0f) {
            return false;
        }
    }
    return true;
}

bool M_SectorContainsCircle(const sector_t *sector, const vector_t *point, float radius) {
    for (size_t i = 0; i < sector->num_walls; i++) {
        const wall_t *wall = &sector->walls[i];
        // Test if circle lies in front of line.
        if (WallPointDist(wall, point) < -radius * wall->length) {
            return false;
        }
    }
    return true;
}

static sector_t *FindPlayerSector(sector_t *sector, const vector_t *point) {
    sectoriter_t iter;
    M_SectorIterNew(&iter, sector);

    // Iterate until we find a sector that contains the player in its walls.
    while ((sector = M_SectorIterPop(&iter))) {
        if (M_SectorContainsPoint(sector, point)) {
            // This sector contains our point.
            M_SectorIterCleanup(&iter);
            return sector;
        }

        // Find any portals and follow them.
        for (size_t i = 0; i < sector->num_walls; i++) {
            const wall_t *wall = &sector->walls[i];
            if (wall->portal != NULL) {
                M_SectorIterPush(&iter, wall->portal);
            }
        }
    }

    // Somehow the player isn't in any sector. As a failsafe, we'll return the
    // sector they were last seen in.
    M_SectorIterCleanup(&iter);
    return iter.root;
}

static float GetCollisionTime(
    const wall_t* wall,
    const vector_t *oc,
    const vector_t *delta
) {
    vector_t c;
    U_VecCopy(&c, oc);
    U_VecScaledAdd(&c, &wall->normal, -OBJ_RADIUS);

    const vector_t *a = wall->v1;
    const vector_t *b = wall->v2;
    float tn = (a->x - c.x) * -delta->y - (a->y - c.y) * -delta->x;
    float un = (a->x - c.x) * (a->y - b->y) - (a->y - c.y) * (a->x - b->x);
    float den = (a->x - b->x) * -delta->y - (a->y - b->y) * -delta->x;
    if (den == 0.0f) {
        return 1.0f;
    }

    float t = tn / den;
    float u = un / den;

    if (u >= -0.01f && u <= 1.0f && t >= -0.01f && t <= 1.0f) {
        return u;
    }

    vector_t da;
    U_VecCopy(&da, oc);
    U_VecSub(&da, a);
    vector_t db;
    U_VecCopy(&db, oc);
    U_VecSub(&db, b);

    // Try to see if there's a point that the left/right endpoints graze the circle.
    float qa = U_VecLenSq(delta);
    if (qa != 0.0f) {
        float qb = U_VecDot(&da, delta);
        float qc = U_VecDot(&da, &da) - (OBJ_RADIUS * OBJ_RADIUS);
        float disc = qb * qb - qa * qc;
        if (disc >= 0.0f) {
            float q = (-qb - sqrtf(disc)) / qa;
            if (q >= -0.01f && q <= 1.0f) {
                return q;
            }
        }
        qb = U_VecDot(&db, delta);
        qc = U_VecDot(&db, &db) - (OBJ_RADIUS * OBJ_RADIUS);
        disc = qb * qb - qa * qc;
        if (disc >= 0.0f) {
            float q = (-qb - sqrtf(disc)) / qa;
            if (q >= -0.01f && q <= 1.0f) {
                return q;
            }
        }
    }

    return 1.0f;
}

static const wall_t *SectorCollide(
    vector_t *pos,
    float zpos,
    vector_t *delta,
    sector_t *sector
) {
    const wall_t *closest = NULL;
    float closest_distance = INFINITY;

    sectoriter_t iter;
    M_SectorIterNew(&iter, sector);

    // AABB of the player.
    aabb_t player_bounds;
    // Get player bounds without movement.
    player_bounds.min.x = pos->x - OBJ_RADIUS;
    player_bounds.min.y = pos->y - OBJ_RADIUS;
    player_bounds.max.x = pos->x + OBJ_RADIUS;
    player_bounds.max.y = pos->y + OBJ_RADIUS;
    // Add bounds based on movement.
    if (delta->x > 0.0f) {
        player_bounds.max.x += delta->x;
    } else {
        player_bounds.min.x += delta->x;
    }
    if (delta->y > 0.0f) {
        player_bounds.max.y += delta->y;
    } else {
        player_bounds.min.y += delta->y;
    }

    while ((sector = M_SectorIterPop(&iter)) != NULL) {
        for (size_t i = 0; i < sector->num_walls; i++) {
            const wall_t *wall = &sector->walls[i];

            // If the wall is a portal, and we overlap it during movement,
            // add it to the queue of portals to check.
            if (wall->portal != NULL) {
                if (AABBOverlap(&player_bounds, &wall->portal->bounds)) {
                    M_SectorIterPush(&iter, wall->portal);
                }

                // If we don't collide with the lower or upper parts of the portal,
                // we will pass through. There's some allowance for colliding with
                // the lower portion, to allow actors to climb stairs.
                if (zpos + OBJECT_HEIGHT < wall->portal->ceiling &&
                    zpos + MAX_STAIR_HEIGHT > wall->portal->floor) {
                    continue;
                }
            }

            if (U_VecDot(&wall->normal, delta) >= 0.0f) {
                continue;
            }

            float t = GetCollisionTime(wall, pos, delta);
            if (t < 1.0f && t < closest_distance) {
                closest = wall;
                closest_distance = t;
            }
        }
    }

    // Check the closest wall collision.
    if (closest != NULL) {
        // We hit the wall. Move position to touch wall.
        U_VecScaledAdd(pos, delta, closest_distance);
        // Do wall sliding.
        vector_t slide;
        U_VecCopy(&slide, closest->v2);
        U_VecSub(&slide, closest->v1);
        U_VecNormalize(&slide);
        float slidefac = U_VecDot(&slide, delta);
        U_VecScale(delta, closest_distance);
        U_VecScaledAdd(delta, &slide, (1.0f - closest_distance) * slidefac);
    }

    M_SectorIterCleanup(&iter);

    return closest;
}

sector_t *M_MoveAndSlide(
    sector_t *sector,
    vector_t *pos,
    float zpos,
    vector_t *delta
) {
    // Only allow so many sector changes.
    uint8_t changes_left = 5;
    while (changes_left-- && U_VecLenSq(delta) > 0.001f) {
        vector_t old_delta;
        U_VecCopy(&old_delta, delta);
        const wall_t *wall = SectorCollide(pos, zpos, delta, sector);
        if (wall != NULL) {
            if (wall->portal == NULL && U_VecDistSq(delta, &old_delta) < 0.001f) {
                break;
            }
            sector = FindPlayerSector(sector, pos);
        } else {
            U_VecAdd(pos, delta);
            sector = FindPlayerSector(sector, pos);
            break;
        }
    }
    // Zero out velocity if it's too small.
    if (U_VecLenSq(delta) <= 0.001f) {
        delta->x = 0.0f;
        delta->y = 0.0f;
    }
    return sector;
}
