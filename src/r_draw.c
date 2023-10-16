#include "b_core.h"
#include "m_iter.h"
#include "r_draw.h"
#include "u_vec.h"

#include <stdbool.h>

// Render position.
static vector_t renderpos;
// Render angle.
static float renderangle;
// Playdate framebuffer.
static uint8_t *__attribute__((aligned(4))) renderbuf;
// Pointer to current column.
static uint8_t *rendercol;
// Mask for current column.
static uint8_t renderxmask;
// The current sector.
static const sector_t *rendersector;
// The current wall.
static const wall_t *renderwall;
// Sine multiply for flats.
static int32_t flatsine;
// Cosine multiply for flats.
static int32_t flatcosine;
// Sine multiply for wall rotation.
static float wallsine;
// Cosine multiply for wall rotation.
static float wallcosine;

static void NewRotatePoint(vector_t *v, float ang) {
    float x = v->x * wallcosine - v->y * wallsine;
    v->y = v->x * wallsine + v->y * wallcosine;
    v->x = x;
}

// Half of the screen X resolution.
#define SCRNDIST 200.0f
// Integer version of SCRNDIST.
#define SCRNDISTI 200

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

// Stride of rows in framebuffer.
#define ROWSTRIDE 52

// y position of floor
#define FLOORHEIGHT -32.0f

// Integer version of FLOORHEIGHT.
#define FLOORHEIGHTI -32

// Draw a floor.
static void DrawFlat(int32_t x, int32_t y, int32_t endy, int32_t offscale) {
    // Avoid division by zero just in case.
    if (y == 120) ++y;
    // Get location in framebuffer to draw to.
    uint8_t *framebuffer = rendercol + (ROWSTRIDE * y);
    int32_t offx = floorf(renderpos.x * 256.0f) * offscale;
    int32_t offy = floorf(renderpos.y * 256.0f) * -offscale;
    int32_t uvx = (SCRNDISTI - x) << 8;
    int32_t uvy = SCRNDISTI << 8;
    int32_t foox = FLOORHEIGHTI * (uvx * flatcosine - uvy * flatsine);
    int32_t fooy = FLOORHEIGHTI * (uvx * flatsine + uvy * flatcosine);
    for (; y < endy; y++) {
        int32_t den = (y - 120) * SCRNDISTI;
        uint16_t newx = ((foox / den + offx) >> 8) & 63;
        uint16_t newy = ((fooy / den + offy) >> 8) & 63;
        uint8_t flatmask = 1 << (7 - (newx & 7));
        uint16_t flatindex = (newx >> 3) + (newy * 8);
        if (rendersector->floor->data[flatindex] & flatmask) {
            *framebuffer |= renderxmask;
        } else {
            *framebuffer &= ~renderxmask;
        }
        framebuffer += ROWSTRIDE;
    }
}

// Draw a column.
static void DrawColumn(
    // Column of patch to draw.
    const uint8_t *source,
    // Height of column patch.
    int32_t scale,
    // The top Y coordinate.
    int32_t y1,
    // The bottom Y coordinate.
    int32_t y2
) {
    // Convert scale to mask.
    scale -= 1;
    int32_t den = y2 - y1;
    int32_t offset = y1;
    if (y1 < 0) y1 = 0;
    if (y2 > 240) y2 = 240;
    uint8_t *framebuffer = rendercol + (ROWSTRIDE * y1);
    // For speed, calculate fixed-point accumulator instead of repeated multiply and divide.
    int32_t fracstep = (scale << 16) / den;
    int32_t frac = ((scale * (y1 - offset)) << 16) / den;
    y2 -= y1;
    while (y2-- > 0) {
        int32_t whichy = (frac >> 16) & scale;
        int32_t ymask = 1 << (7 - (whichy & 7));
        int32_t yindex = whichy >> 3;
        if (source[yindex] & ymask) {
            *framebuffer |= renderxmask;
        } else {
            *framebuffer &= ~renderxmask;
        }
        frac += fracstep;
        framebuffer += ROWSTRIDE;
    }
}

static void DrawWallColumns(
    int32_t x1,
    int32_t y1,
    int32_t x2,
    int32_t y2,
    float ua,
    float ub,
    float za,
    float zb
) {
    // Sort by X position for easier clipping.
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
    rendercol = &renderbuf[x1 >> 3];
    renderxmask = 1 << (7 - (x1 & 7));
    int32_t den = x2 - x1;
    int32_t num = y2 - y1;
    x2 -= x1;
    int32_t ua1 = floorf(ua * 256.0f);
    int32_t ub1 = floorf(ub * 256.0f);
    int32_t za1 = floorf(za);
    int32_t zb1 = floorf(zb);
    int32_t du = ub1 - ua1;
    int32_t dz = zb1 - za1;
    const patch_t *patch = renderwall->patch;
    for (int32_t x = 0; x < x2; x++) {
        int32_t num1 = du * (za1 * x);
        int32_t den1 = (zb1 * x2) - x * dz;
        int32_t whichx = ((ua1 + (num1 / den1)) >> 8) & (patch->width - 1);
        int32_t y = y1 + (x * num) / den;
        int32_t ay1 = 120 - y;
        int32_t ay2 = 120 + y;
        DrawFlat(x + x1, 0, ay1, -1);
        DrawColumn(
            &patch->data[whichx * patch->stride],
            patch->height,
            ay1,
            ay2
        );
        DrawFlat(x + x1, ay2, 240, 1);
        renderxmask >>= 1;
        if (renderxmask == 0) {
            ++rendercol;
            renderxmask = 0x80;
        }
    }
}

static bool DrawWall(float *left, float *right) {
    // Find the corners of this wall.
    vector_t a, b;
    U_VecCopy(&a, renderwall->v1);
    U_VecCopy(&b, renderwall->v2);
    U_VecSub(&a, &renderpos);
    U_VecSub(&b, &renderpos);
    NewRotatePoint(&a, renderangle);
    NewRotatePoint(&b, renderangle);
    // Find UVs of this wall.
    float ua = renderwall->xoffset;
    float ub = ua + renderwall->length;
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
    float pua = ua + ta * (ub - ua);
    float pub = ua + tb * (ub - ua);
    if (var1 > caa) {
        if (var1 > cba) {
            a.x = pbx;
            a.y = pby;
            nca = cb;
            ua = pub;
        } else {
            nca = var1 / nay;
        }
    } else {
        a.x = pax;
        a.y = pay;
        nca = ca;
        ua = pua;
    }
    if (a.y < 0.0f) {
        return false;
    }
    if (var2 < cbb) {
        if (var2 < cab) {
            b.x = pax;
            b.y = pay;
            ncb = ca;
            ub = pua;
        } else {
            ncb = var2 / nby;
        }
    } else {
        b.x = pbx;
        b.y = pby;
        ncb = cb;
        ub = pub;
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
    if (renderwall->portal != NULL) {
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
    DrawWallColumns(axi, ayi, bxi, byi, ua, ub, a.y, b.y);
    return false;
}

static void DrawSectorRecursive(sector_t *sector, float left, float right) {
    rendersector = sector;
    // Check each wall in the sector.
    for (size_t i = 0; i < sector->num_walls; i++) {
        renderwall = &sector->walls[i];
        // Bounds of wall.
        float nleft = left;
        float nright = right;
        // Check if portal should be drawn.
        bool draw_portal = DrawWall(&nleft, &nright);
        if (draw_portal && fabsf(nleft - nright) >= 0.5f) {
            // If it should be drawn, draw it recursively using clipped bounds.
            DrawSectorRecursive(renderwall->portal, nleft, nright);
        }
    }
}

void R_DrawSector(sector_t *sector, const vector_t *pos, float ang) {
    // Set globals.
    U_VecCopy(&renderpos, pos);
    renderangle = ang;
    wallsine = sinf(-renderangle);
    wallcosine = cosf(-renderangle);
    flatsine = floorf(wallsine * SCRNDIST);
    flatcosine = floorf(wallcosine * SCRNDIST);
    renderbuf = playdate->graphics->getFrame();
    // Run recursive sector drawing.
    DrawSectorRecursive(sector, -200.0f, 200.0f);
    // Notify the system that we've drawn over the framebuffer.
    playdate->graphics->markUpdatedRows(0, LCD_ROWS - 1);
}
