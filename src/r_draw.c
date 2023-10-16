#include "b_core.h"
#include "m_iter.h"
#include "r_draw.h"
#include "u_vec.h"

#include <stdbool.h>

static void RotatePoint(float *x, float *y, float ang) {
    // TODO inefficient - sin and cos are recalculated way too often
    float s = sinf(-ang);
    float c = cosf(-ang);
    float new_x  = *x * c - *y * s;
    *y = *x * s + *y * c;
    *x = new_x;
}

static void NewRotatePoint(vector_t *v, float ang) {
    // TODO inefficient - sin and cos are recalculated way too often
    float s = sinf(-ang);
    float c = cosf(-ang);
    float x = v->x * c - v->y * s;
    v->y = v->x * s + v->y * c;
    v->x = x;
}

// Set to 0 when drawing starts. Each bit represents a column that has been drawn in.
// This is used to prevent overdraw.
static uint8_t FilledColumns[LCD_COLUMNS >> 3];

// Half of the screen X resolution.
#define SCRNDIST 200.0f

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

static void DrawWallColumns(
    uint8_t *framebuffer,
    int32_t x1,
    int32_t y1,
    int32_t x2,
    int32_t y2
) {
    // Sort byi X position for easier clipping.
    if (x1 > x2) {
        int32_t temp;
        temp = x1;
        x1 = x2;
        x2 = temp;
        temp = y1;
        y1 = y2;
        y2 = temp;
    }
    // Shouldn't happen, but good to check in case of rounding errors.
    if (x1 < -200) {
        x1 = -200;
    }
    if (x2 > 200) {
        x2 = 200;
    }
    // Convert to screen position.
    x1 += 200;
    x2 += 200;
    // Draw columns.
    uint8_t *buf = &framebuffer[x1 >> 3];
    uint8_t mask1 = 1 << (7 - (x1 & 7));
    int32_t den = x2 - x1;
    int32_t num = y2 - y1;
    for (int32_t x = x1; x < x2; x++) {
        // int mask = 1 << (x & 7);
        int32_t y = y1 + ((x - x1) * num) / den;
        // Figure out temporary shade of column.
        int shade = y >> 3;
        if (shade < 0) {
            shade = 0;
        } else if (shade > 16) {
            shade = 16;
        }
        int ay1 = 120 - y;
        int ay2 = 120 + y;
        if (ay1 < 0) {
            ay1 = 0;
        }
        if (ay2 > 239) {
            ay2 = 239;
        }
        uint8_t *col = &buf[52 * ay1];
        for (y = ay1; y <= ay2; y++) {
            *col |= mask1 & shades[shade][y & 3];
            col += 52;
        }
        mask1 >>= 1;
        if (mask1 == 0) {
            ++buf;
            mask1 = 0x80;
        }
    }
}

static bool DrawWall(
    uint8_t *framebuffer,
    const wall_t *wall,
    const vector_t *pos,
    float ang,
    float *left,
    float *right
) {
    // Find the corners of this wall.
    vector_t a, b;
    U_VecCopy(&a, wall->v1);
    U_VecCopy(&b, wall->v2);
    U_VecSub(&a, pos);
    U_VecSub(&b, pos);
    NewRotatePoint(&a, ang);
    NewRotatePoint(&b, ang);
    // TODO clean up this code.
    vector_t d;
    U_VecCopy(&d, &b);
    U_VecSub(&d, &a);
    float ca = *left;
    float cb = *right;
    float var1 = SCRNDIST * a.x;
    float var2 = SCRNDIST * b.x;
    float caa = ca * a.y;
    float cab = ca * b.y;
    float cba = cb * a.y;
    float cbb = cb * b.y;
    if ((var1 < caa && var2 < cab) || (var1 > cba && var2 > cbb)) {
        return false;
    }
    float nay = a.y;
    float nby = b.y;
    float nca, ncb;
    float ta = (caa - var1) / (SCRNDIST * d.x - ca * d.y);
    float tb = (cba - var1) / (SCRNDIST * d.x - cb * d.y);
    float pax = a.x + ta * d.x;
    float pay = a.y + ta * d.y;
    float pbx = a.x + tb * d.x;
    float pby = a.y + tb * d.y;
    if (var1 > caa) {
        if (var1 > cba) {
            a.x = pbx;
            a.y = pby;
            nca = cb;
        } else {
            nca = var1 / nay;
        }
    } else {
        a.x = pax;
        a.y = pay;
        nca = ca;
    }
    if (a.y < 0.0f) {
        return false;
    }
    if (var2 < cbb) {
        if (var2 < cab) {
            b.x = pax;
            b.y = pay;
            ncb = ca;
        } else {
            ncb = var2 / nby;
        }
    } else {
        b.x = pbx;
        b.y = pby;
        ncb = cb;
    }
    if (nca > ncb) {
        float temp = nca;
        nca = ncb;
        ncb = temp;
    }
    if (b.y < 0.0f) {
        return false;
    }
    // Backface culling.
    if ((a.x * b.y) > (b.x * a.y)) {
        return false;
    }
    // If this is a portal, don't render it.
    if (wall->portal != NULL) {
        *left = nca;
        *right = ncb;
        return true;
    }
    // Project onto screen using 90-degree FOV.
    int32_t axi = floorf(nca);
    int32_t bxi = floorf(ncb);
    int32_t ayi = floorf((SCRNDIST * 32.0f) / a.y);
    int32_t byi = floorf((SCRNDIST * 32.0f) / b.y);
    // Draw columns.
    DrawWallColumns(framebuffer, axi, ayi, bxi, byi);
    return false;
}

bool DrawWallAutoMap(const wall_t *wall, const vector_t *pos, float ang) {
    // Find the corners of this wall.
    float ax = wall->v1->x - pos->x;
    float ay = wall->v1->y - pos->y;
    RotatePoint(&ax, &ay, ang);
    float bx = wall->v2->x - pos->x;
    float by = wall->v2->y - pos->y;
    RotatePoint(&bx, &by, ang);
    // Naive line drawing for now.
    if (wall->portal != NULL) {
        return true;
    } else {
        playdate->graphics->drawLine(ax + 200.0f, 120.0f - ay, bx + 200.0f, 120.0f - by, 1, kColorWhite);
        return false;
    }
}

static void DrawSectorRecursive(
    uint8_t *framebuffer,
    sector_t *sector,
    const vector_t *pos,
    float ang,
    float left,
    float right
) {
    // Check each wall in the sector.
    for (size_t i = 0; i < sector->num_walls; i++) {
        const wall_t *wall = &sector->walls[i];
        // Bounds of wall.
        float nleft = left;
        float nright = right;
        // Check if portal should be drawn.
        bool draw_portal = DrawWall(framebuffer, wall, pos, ang, &nleft, &nright);
        if (draw_portal && fabsf(nleft - nright) >= 0.5f) {
            // If it should be drawn, draw it recursively using clipped bounds.
            DrawSectorRecursive(framebuffer, wall->portal, pos, ang, nleft, nright);
        }
    }
}

void R_DrawSector(uint8_t *framebuffer, sector_t *sector, const vector_t *pos, float ang) {
    // Clear all columns.
    memset(FilledColumns, 0, sizeof(FilledColumns));
    DrawSectorRecursive(framebuffer, sector, pos, ang, -200.0f, 200.0f);
    // Notify the system that we've drawn over the framebuffer.
    playdate->graphics->markUpdatedRows(0, LCD_ROWS - 1);
}

void R_DrawAutoMap(sector_t *sector, const vector_t *pos, float ang) {
    // Iterate.
    sectoriter_t iter;
    M_SectorIterNew(&iter, sector);
    // While the queue is not empty...
    while ((sector = M_SectorIterPop(&iter)) != NULL) {
        // Check each wall in the queue.
        for (size_t i = 0; i < sector->num_walls; i++) {
            const wall_t *wall = &sector->walls[i];
            bool draw_portal = DrawWallAutoMap(wall, pos, ang);
            if (draw_portal) {
                // If the wall is a portal, push it.
                sector_t *portal = wall->portal;
                M_SectorIterPush(&iter, portal);
            }
        }
    }
    // Clean up linked lists.
    M_SectorIterCleanup(&iter);
    // Draw a circle in the center to debug collision.
    playdate->graphics->drawEllipse(180, 100, 40, 40, 1, 0.0f, 0.0f, kColorWhite);
}
