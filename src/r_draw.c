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
// Pointer to current filledbits column.
static uint8_t *fillcol;
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
// Left X coordinate of wall rendering, inclusive.
static int32_t renderxmin;
// Right X coordinate of wall rendering, exclusive.
static int32_t renderxmax;

// Bitmask of written pixels. While we want to minimize overdraw for speed, it
// could happen due to rounding error, or some parts may not yet support no overdraw.
static uint8_t filledbits[52 * 240];

static void NewRotatePoint(vector_t *v, float ang) {
    float x = v->x * wallcosine - v->y * wallsine;
    v->y = v->x * wallsine + v->y * wallcosine;
    v->x = x;
}

// Half of the screen X resolution.
#define SCRNDIST 200.0f
// Integer version of SCRNDIST.
#define SCRNDISTI 200

// Eye level above ground.
#define EYELEVEL 32

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

// Draw a floor.
static void DrawFlat(int32_t x, int32_t y, int32_t endy, int height, int32_t offscale) {
    if (y < 0) y = 0;
    if (endy > 240) endy = 240;
    // Get location in framebuffer to draw to.
    uint8_t *framebuffer = rendercol + (ROWSTRIDE * y);
    uint8_t *fillbuffer = fillcol + (ROWSTRIDE * y);
    int32_t offx = floorf(renderpos.x * 256.0f) * offscale;
    int32_t offy = floorf(renderpos.y * 256.0f) * -offscale;
    int32_t uvx = (SCRNDISTI - x) << 8;
    int32_t uvy = SCRNDISTI << 8;
    int32_t foox = height * (uvx * flatcosine - uvy * flatsine);
    int32_t fooy = height * (uvx * flatsine + uvy * flatcosine);
    for (; y < endy; y++) {
        int32_t den = (y - 120) * SCRNDISTI;
        if (den != 0) {
            uint16_t newx = ((foox / den + offx) >> 8) & 63;
            uint16_t newy = ((fooy / den + offy) >> 8) & 63;
            uint8_t flatmask = 1 << (7 - (newx & 7));
            uint16_t flatindex = (newx >> 3) + (newy * 8);
            if (!(*fillbuffer & renderxmask)) {
                *fillbuffer |= renderxmask;
                if (rendersector->floorflat->data[flatindex] & flatmask) {
                    *framebuffer |= renderxmask;
                } else {
                    *framebuffer &= ~renderxmask;
                }
            }
        }
        framebuffer += ROWSTRIDE;
        fillbuffer += ROWSTRIDE;
    }
}

// Draw a column.
static void DrawColumn(
    // Column of patch to draw.
    const uint8_t *source,
    // Height of column patch.
    int32_t scale,
    // Height of wall.
    int32_t height,
    // The top Y coordinate.
    int32_t y1,
    // The bottom Y coordinate.
    int32_t y2
) {
    int32_t den = y2 - y1;
    int32_t offset = y1;
    if (y1 < 0) y1 = 0;
    if (y2 > 240) y2 = 240;
    uint8_t *framebuffer = rendercol + (ROWSTRIDE * y1);
    uint8_t *fillbuffer = fillcol + (ROWSTRIDE * y1);
    // For speed, calculate fixed-point accumulator instead of repeated multiply and divide.
    int32_t fracstep = (height << 12) / den;
    int32_t frac = ((height * (y1 - offset)) << 12) / den;
    // Convert scale to mask.
    scale -= 1;
    y2 -= y1;
    while (y2-- > 0) {
        int32_t whichy = (frac >> 12) & scale;
        int32_t ymask = 1 << (7 - (whichy & 7));
        int32_t yindex = whichy >> 3;
        if (!(*fillbuffer & renderxmask)) {
            *fillbuffer |= renderxmask;
            if (source[yindex] & ymask) {
                *framebuffer |= renderxmask;
            } else {
                *framebuffer &= ~renderxmask;
            }
        }
        frac = (frac + fracstep) & ((scale << 12) | 0xfff);
        framebuffer += ROWSTRIDE;
        fillbuffer += ROWSTRIDE;
    }
}

static void DrawWallColumns(
    int32_t y1h,
    int32_t y1l,
    int32_t y2h,
    int32_t y2l,
    bool drawwall,
    bool drawflat,
    int32_t cf,
    int32_t ff,
    float ua,
    float ub,
    float za,
    float zb
) {
    int32_t x1 = renderxmin;
    int32_t x2 = renderxmax;
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
    fillcol = &filledbits[x1 >> 3];
    renderxmask = 1 << (7 - (x1 & 7));
    int32_t den = x2 - x1;
    int32_t numh = y2h - y1h;
    int32_t numl = y2l - y1l;
    x2 -= x1;
    int32_t ua1 = floorf(ua * 4.0f);
    int32_t ub1 = floorf(ub * 4.0f);
    int32_t za1 = floorf(za);
    int32_t zb1 = floorf(zb);
    if (zb1 == 0) {
        // We'll divide by zero if we let this wall render.
        return;
    }
    int32_t du = ub1 - ua1;
    int32_t dz = zb1 - za1;
    const patch_t *patch = renderwall->patch;
    for (int32_t x = 0; x < x2; x++) {
        int32_t num1 = du * (za1 * x);
        int32_t den1 = (zb1 * x2) - x * dz;
        int32_t whichx = ((ua1 + (num1 / den1)) >> 2) & (patch->width - 1);
        int32_t yh = y1h + (x * numh) / den;
        int32_t yl = y1l + (x * numl) / den;
        int32_t ay1 = yh + 120;
        int32_t ay2 = yl + 120;
        if (drawwall) {
            DrawColumn(
                &patch->data[whichx * patch->stride],
                patch->height,
                ff - cf,
                ay1,
                ay2
            );
        }
        if (drawflat) {
            DrawFlat(x + x1, 0, ay1, cf, -1);
            DrawFlat(x + x1, ay2, 240, ff, -1);
        }
        renderxmask >>= 1;
        if (renderxmask == 0) {
            ++rendercol;
            ++fillcol;
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
    renderxmin = floorf(nca);
    renderxmax = floorf(ncb);

    int32_t ayi = floorf(a.y);
    int32_t byi = floorf(b.y);
    if (ayi == 0) ayi = 1;
    if (byi == 0) byi = 1;

    int32_t ayih = (SCRNDISTI * (EYELEVEL - rendersector->ceiling)) / ayi;
    int32_t byih = (SCRNDISTI * (EYELEVEL - rendersector->ceiling)) / byi;
    int32_t ayil = (SCRNDISTI * (EYELEVEL - rendersector->floor)) / ayi;
    int32_t byil = (SCRNDISTI * (EYELEVEL - rendersector->floor)) / byi;

    if (renderwall->portal != NULL) {
        *left = nca;
        *right = ncb;

        DrawWallColumns(ayih, ayil, byih, byil, false, true, (EYELEVEL - rendersector->ceiling), (EYELEVEL - rendersector->floor), ua, ub, a.y, b.y);

        // If the height of the ceiling goes down, render top wall.
        if (renderwall->portal->ceiling < rendersector->ceiling) {
            ayih = (SCRNDISTI * (EYELEVEL - rendersector->ceiling)) / ayi;
            byih = (SCRNDISTI * (EYELEVEL - rendersector->ceiling)) / byi;
            ayil = (SCRNDISTI * (EYELEVEL - renderwall->portal->ceiling)) / ayi;
            byil = (SCRNDISTI * (EYELEVEL - renderwall->portal->ceiling)) / byi;
            DrawWallColumns(ayih, ayil, byih, byil, true, false, (EYELEVEL - rendersector->ceiling), (EYELEVEL - renderwall->portal->ceiling), ua, ub, a.y, b.y);
        }

        // If the height of the floor goes up, render bottom wall.
        if (renderwall->portal->floor > rendersector->floor) {
            ayih = (SCRNDISTI * (EYELEVEL - renderwall->portal->floor)) / ayi;
            byih = (SCRNDISTI * (EYELEVEL - renderwall->portal->floor)) / byi;
            ayil = (SCRNDISTI * (EYELEVEL - rendersector->floor)) / ayi;
            byil = (SCRNDISTI * (EYELEVEL - rendersector->floor)) / byi;
            DrawWallColumns(ayih, ayil, byih, byil, true, false, (EYELEVEL - renderwall->portal->floor), (EYELEVEL - rendersector->floor), ua, ub, a.y, b.y);
        }

        return true;
    } else {
        DrawWallColumns(ayih, ayil, byih, byil, true, true, (EYELEVEL - rendersector->ceiling), (EYELEVEL - rendersector->floor), ua, ub, a.y, b.y);
        return false;
    }
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
            // Restore globals.
            rendersector = sector;
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
    memset(filledbits, 0, sizeof(filledbits));
    // Run recursive sector drawing.
    DrawSectorRecursive(sector, -200.0f, 200.0f);
    // Notify the system that we've drawn over the framebuffer.
    playdate->graphics->markUpdatedRows(0, LCD_ROWS - 1);
}
