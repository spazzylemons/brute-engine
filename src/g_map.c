#include "b_math.h"
#include "core.h"
#include "g_map.h"

#include <math.h>

#define OBJ_RADIUS 20.0f

static sector_t *FindPlayerSector(sector_t *sector, const vertex_t *point) {
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
    const vertex_t *oc,
    const vertex_t *delta
) {
    vertex_t c;
    B_VtxCopy(&c, oc);
    B_VtxScaledAdd(&c, &wall->normal, -OBJ_RADIUS);

    const vertex_t *a = wall->v1;
    const vertex_t *b = wall->v2;
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

    vertex_t da;
    B_VtxCopy(&da, oc);
    B_VtxSub(&da, a);
    vertex_t db;
    B_VtxCopy(&db, oc);
    B_VtxSub(&db, b);

    // Try to see if there's a point that the left/right endpoints graze the circle.
    float qa = B_VtxLenSq(delta);
    if (qa != 0.0f) {
        float qb = B_VtxDot(&da, delta);
        float qc = B_VtxDot(&da, &da) - (OBJ_RADIUS * OBJ_RADIUS);
        float disc = qb * qb - qa * qc;
        if (disc >= 0.0f) {
            float q = (-qb - sqrtf(disc)) / qa;
            if (q >= -0.01f && q <= 1.0f) {
                return q;
            }
        }
        qb = B_VtxDot(&db, delta);
        qc = B_VtxDot(&db, &db) - (OBJ_RADIUS * OBJ_RADIUS);
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
    vertex_t *pos,
    vertex_t *delta,
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

            // Don't collide with portals. Instead, check their sectors if we
            // may collide with the sector's walls.
            if (wall->portal != NULL) {
                if (B_AABBOverlap(&player_bounds, &wall->portal->bounds)) {
                    M_SectorIterPush(&iter, wall->portal);
                }
                continue;
            }

            if (B_VtxDot(&wall->normal, delta) >= 0.0f) {
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
        B_VtxScaledAdd(pos, delta, closest_distance);
        // Do wall sliding.
        vertex_t slide;
        B_VtxCopy(&slide, closest->v2);
        B_VtxSub(&slide, closest->v1);
        B_VtxNormalize(&slide);
        float slidefac = B_VtxDot(&slide, delta);
        B_VtxScale(delta, closest_distance);
        B_VtxScaledAdd(delta, &slide, (1.0f - closest_distance) * slidefac);
    }

    M_SectorIterCleanup(&iter);

    return closest;
}

sector_t *G_MoveAndSlide(sector_t *sector, vertex_t *pos, vertex_t *delta) {
    // failsafe - only allow so many sector changes
    // this hasn't been triggered to my knowledge and may not be useful
    uint8_t changes_left = 5;
    while (changes_left-- && B_VtxLenSq(delta) > 0.0001f) {
        vertex_t old_delta;
        B_VtxCopy(&old_delta, delta);
        const wall_t *wall = SectorCollide(pos, delta, sector);
        if (wall != NULL) {
            if (wall->portal == NULL && B_VtxDistSq(delta, &old_delta) < 0.0001f) {
                break;
            }
            sector = FindPlayerSector(sector, pos);
        } else {
            B_VtxAdd(pos, delta);
            sector = FindPlayerSector(sector, pos);
            break;
        }
    }
    if (changes_left == 0) {
        playdate->system->logToConsole("Too many wall slides!");
    }
    return sector;
}
