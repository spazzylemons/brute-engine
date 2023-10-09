#include "core.h"
#include "m_fixed.h"
#include "m_trig.h"
#include "r_draw.h"

#include <stdbool.h>

static void RotatePoint(fixed_t *x, fixed_t *y, angle_t ang) {
    // TODO inefficient - sin and cos are recalculated way too often
    fixed_t s = M_Sine(ang);
    fixed_t c = M_Cosine(ang);
    fixed_t new_x  = M_FixedMul(*x, c) - M_FixedMul(*y, s);
    *y = M_FixedMul(*x, s) + M_FixedMul(*y, c);
    *x = new_x;
}

// Set to 0 when drawing starts. Each bit represents a column that has been drawn in.
// This is used to prevent overdraw.
static uint8_t FilledColumns[LCD_COLUMNS >> 3];

// Half of the screen X resolution.
#define SCREEN_DISTANCE 200

#define NEAR_PLANE (1 << FRACBITS)

#define DitherPattern(a, b, c, d) { a * 0x11, b * 0x11, c * 0x11, d * 0x11 }

static const uint8_t shades[17][4] = {
    DitherPattern(0x0, 0x0, 0x0, 0x0),
    DitherPattern(0x8, 0x0, 0x0, 0x0),
    DitherPattern(0x8, 0x0, 0x2, 0x0),
    DitherPattern(0x8, 0x0, 0xa, 0x0),
    DitherPattern(0xa, 0x0, 0xa, 0x0),
    DitherPattern(0xa, 0x0, 0xa, 0x1),
    DitherPattern(0xa, 0x4, 0xa, 0x1),
    DitherPattern(0xa, 0x4, 0xa, 0x5),
    DitherPattern(0xa, 0x5, 0xa, 0x5),
    DitherPattern(0xd, 0x5, 0xa, 0x5),
    DitherPattern(0xd, 0x5, 0xe, 0x5),
    DitherPattern(0xd, 0x5, 0xf, 0x5),
    DitherPattern(0xf, 0x5, 0xf, 0x5),
    DitherPattern(0xf, 0x5, 0xf, 0xd),
    DitherPattern(0xf, 0x7, 0xf, 0xd),
    DitherPattern(0xf, 0x7, 0xf, 0xf),
    DitherPattern(0xf, 0xf, 0xf, 0xf),
};

static bool DrawWall(uint8_t *framebuffer, const wall_t *wall, const vertex_t *pos, angle_t ang) {
    // Find the corners of this wall.
    fixed_t ax = wall->v1->x - pos->x;
    fixed_t ay = wall->v1->y - pos->y;
    RotatePoint(&ax, &ay, ang);
    fixed_t bx = wall->v2->x - pos->x;
    fixed_t by = wall->v2->y - pos->y;
    RotatePoint(&bx, &by, ang);
    // Frustrum culling.
    if (ax > ay && bx > by) {
        return false;
    }
    // Sort by distance to camera for "Z" (techinically Y) clipping.
    bool flipped = false;
    if (ay > by) {
        fixed_t temp;
        temp = ax;
        ax = bx;
        bx = temp;
        temp = ay;
        ay = by;
        by = temp;
        flipped = true;
    }
    // Check if the line needs to be clipped.
    if (ay < NEAR_PLANE) {
        if (by < NEAR_PLANE) {
            // Entirely behind near plane.
            return false;
        }
        // Partially behind near plane. Clip it.
        fixed_t t = M_FixedDiv(NEAR_PLANE - ay, by - ay);
        ax += M_FixedMul(t, bx - ax);
        ay = NEAR_PLANE;
    }

    // Project onto screen using 90-degree FOV.
    // From this point forward, we're dealing in integer values and not fixed-point values.
    // We just reuse the fixed-point variables since they're the same underlying type.
    ax = ((SCREEN_DISTANCE * (ax >> FRACBITS)) / (ay >> FRACBITS));
    bx = ((SCREEN_DISTANCE * (bx >> FRACBITS)) / (by >> FRACBITS));
    ay = ((SCREEN_DISTANCE * (32)) / (ay >> FRACBITS));
    by = ((SCREEN_DISTANCE * (32)) / (by >> FRACBITS));
    // Backface culling.
    if ((ax < bx) == flipped) {
        return false;
    }
    // Sort by X position for easier clipping.
    if (ax > bx) {
        int32_t temp;
        temp = ax;
        ax = bx;
        bx = temp;
        temp = ay;
        ay = by;
        by = temp;
    }
    // Check if should clip to the left.
    if (ax < -200) {
        if (bx < -200) {
            // Fully off screen to the left.
            return false;
        }
        ay += ((-200 - ax) * (by - ay)) / (bx - ax);
        ax = -200;
    }
    if (bx > 199) {
        if (ax > 199) {
            // Fully off screen to the right.
            return false;
        }
        by += ((199 - bx) * (ay - by)) / (ax - bx);
        bx = 199;
    }
    // If wall is a portal, this is our cue to exit.
    if (wall->portal != NULL) {
        return true;
    }

    // Get points of polygon to draw.
    int x1 = ax + 200;
    int x2 = bx + 200;
    // Draw rectangle.
    uint8_t *buf = &framebuffer[x1 >> 3];
    uint8_t mask1 = 1 << (7 - (x1 & 7));
    for (int x = x1; x <= x2; x++) {
        int index = x >> 3;
        int mask = 1 << (x & 7);
        if ((FilledColumns[index] & mask) == 0) {
            FilledColumns[index] |= mask;
            // This division is designed specifically to avoid dividing by zero.
            int y = ay + (((x - x1) * (by - ay)) / (x2 - x1 + 1));
            // Figure out temporary shade of column.
            int shade = y >> 3;
            if (shade < 0) {
                shade = 0;
            } else if (shade > 16) {
                shade = 16;
            }
            int y1 = 120 - y;
            int y2 = 120 + y;
            if (y1 < 0) {
                y1 = 0;
            }
            if (y2 > 239) {
                y2 = 239;
            }
            uint8_t *col = &buf[52 * y1];
            for (y = y1; y <= y2; y++) {
                *col |= (mask1 & shades[shade][y & 3]);
                col += 52;
            }
        }
        mask1 >>= 1;
        if (mask1 == 0) {
            ++buf;
            mask1 = 0x80;
        }
        // playdate->graphics->drawLine(x, 120 - y, x, 120 + y, 1, (LCDColor) &shades[shade]);
    }
    return false;
}

void R_DrawSector(uint8_t *framebuffer, sector_t *sector, const vertex_t *pos, angle_t ang) {
    // Clear all columns.
    memset(FilledColumns, 0, sizeof(FilledColumns));
    // Remember the first sector.
    sector_t *root = sector;
    // Keep a list of all drawn sectors.
    sector_t *drawn = sector;
    // Add it to the queue.
    sector_t *queue = sector;
    // While the queue is not empty...
    while (queue != NULL) {
        // Pick off a sector from the queue.
        sector = queue;
        queue = queue->next_queued;
        // Check each wall in the queue.
        for (size_t i = 0; i < sector->num_walls; i++) {
            const wall_t *wall = &sector->walls[i];
            bool draw_portal = DrawWall(framebuffer, wall, pos, ang);
            if (draw_portal) {
                // If the wall is a portal...
                sector_t *portal = wall->portal;
                // First, we check if the portal sector has been drawn already.
                // It's drawn if it is either the root or has a non-NULL next_drawn pointer.
                if (portal != root && portal->next_drawn == NULL) {
                    // Mark the sector as "drawn" as to not add it to the queue any more.
                    portal->next_drawn = drawn;
                    drawn = portal;
                    // Now add it to the queue. It acts more like a stack but is functionally a
                    // queue for our purposes.
                    portal->next_queued = queue;
                    queue = portal;
                }
            }
        }
    }
    // Clean up linked lists.
    while (drawn != NULL) {
        sector = drawn;
        drawn = drawn->next_drawn;
        sector->next_drawn = NULL;
        sector->next_queued = NULL;
    }
}
