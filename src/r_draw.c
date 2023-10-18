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
// Left X coordinate of wall rendering, inclusive.
static int32_t renderxmin;
// Right X coordinate of wall rendering, exclusive.
static int32_t renderxmax;
// Height to draw ceiling at.
static int32_t heightceiling;
// Height to draw floor at.
static int32_t heightfloor;
// Distance of left side of wall.
static int32_t distleft;
// Distance of right side of wall.
static int32_t distright;
// Left UV X coordinate.
static int32_t uvleft;
// Right UV X coordinate.
static int32_t uvright;
// Eye height to render at.
static float rendereyeheight;

// Enum for how to clip sector bounds.
typedef enum {
    CLIP_NONE,  // Perform no clipping.
    CLIP_CEIL,  // Clip the ceiling Y bounds.
    CLIP_FLOOR, // Clip the floor Y bounds.
} cliptype_t;

static uint8_t renderminy[LCD_COLUMNS];
static uint8_t rendermaxy[LCD_COLUMNS];

// static uint8_t *const clipbounds[] = {
//     [CLIP_NONE] = NULL,
//     [CLIP_CEIL] = renderminy,
//     [CLIP_FLOOR] = rendermaxy,
// };

static void TryClip(cliptype_t cliptype, int32_t val, int32_t x) {
    if (val < 0) {
        val = 0;
    } else if (val > LCD_ROWS) {
        val = LCD_ROWS;
    }

    switch (cliptype) {
        case CLIP_NONE:
            break;
        case CLIP_CEIL:
            if (val > renderminy[x]) {
                renderminy[x] = val;
            }
            break;
        case CLIP_FLOOR:
            if (val < rendermaxy[x]) {
                rendermaxy[x] = val;
            }
            break;
    }
}

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
    if (y < renderminy[x]) y = renderminy[x];
    if (endy > rendermaxy[x]) endy = rendermaxy[x];
    // Get location in framebuffer to draw to.
    uint8_t *framebuffer = rendercol + (ROWSTRIDE * y);
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
            if (rendersector->floorflat->data[flatindex] & flatmask) {
                *framebuffer |= renderxmask;
            } else {
                *framebuffer &= ~renderxmask;
            }
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
    // Height of wall.
    int32_t height,
    // The x coordinate.
    int32_t x,
    // The top Y coordinate.
    int32_t y1,
    // The bottom Y coordinate.
    int32_t y2
) {
    int32_t den = y2 - y1;
    int32_t offset = y1;
    if (y1 < renderminy[x]) y1 = renderminy[x];
    if (y2 > rendermaxy[x]) y2 = rendermaxy[x];
    uint8_t *framebuffer = rendercol + (ROWSTRIDE * y1);
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
        if (source[yindex] & ymask) {
            *framebuffer |= renderxmask;
        } else {
            *framebuffer &= ~renderxmask;
        }
        frac = (frac + fracstep) & ((scale << 12) | 0xfff);
        framebuffer += ROWSTRIDE;
    }
}

static void DrawWallColumns(
    bool drawwall,
    bool drawflat,
    cliptype_t ceilclip,
    cliptype_t floorclip
) {
    int32_t x1 = renderxmin;
    int32_t x2 = renderxmax;
    int32_t y1h = (SCRNDISTI * heightceiling) / distleft;
    int32_t y2h = (SCRNDISTI * heightceiling) / distright;
    int32_t y1l = (SCRNDISTI * heightfloor) / distleft;
    int32_t y2l = (SCRNDISTI * heightfloor) / distright;
    // Draw columns.
    rendercol = &renderbuf[x1 >> 3];
    renderxmask = 1 << (7 - (x1 & 7));
    int32_t den = x2 - x1;
    int32_t numh = y2h - y1h;
    int32_t numl = y2l - y1l;
    x2 -= x1;
    int32_t du = uvright - uvleft;
    int32_t dz = distright - distleft;
    const patch_t *patch = renderwall->patch;
    for (int32_t x = 0; x < x2; x++) {
        int32_t num1 = du * (distleft * x);
        int32_t den1 = (distright * x2) - x * dz;
        int32_t whichx = ((uvleft + (num1 / den1)) >> 2) & (patch->width - 1);
        int32_t yh = y1h + (x * numh) / den;
        int32_t yl = y1l + (x * numl) / den;
        int32_t ay1 = yh + 120;
        int32_t ay2 = yl + 120;
        if (drawwall) {
            DrawColumn(
                &patch->data[whichx * patch->stride],
                patch->height,
                heightfloor - heightceiling,
                x + x1,
                ay1,
                ay2
            );
        }
        if (drawflat) {
            DrawFlat(x + x1, 0, ay1, heightceiling, -1);
            DrawFlat(x + x1, ay2, 240, heightfloor, -1);
        }
        TryClip(ceilclip, ay1, x + x1);
        TryClip(floorclip, ay2, x + x1);
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

    renderxmin = floorf(nca) + 200;
    if (renderxmin < 0) {
        renderxmin = 0;
    } else if (renderxmin > 400) {
        renderxmin = 400;
    }

    renderxmax = floorf(ncb) + 200;
    if (renderxmax < 0) {
        renderxmax = 0;
    } else if (renderxmax > 400) {
        renderxmax = 400;
    }

    int32_t ayi = floorf(a.y);
    int32_t byi = floorf(b.y);
    if (ayi == 0) ayi = 1;
    if (byi == 0) byi = 1;
    distleft = ayi;
    distright = byi;

    heightceiling = rendereyeheight - rendersector->ceiling;
    heightfloor = rendereyeheight - rendersector->floor;

    uvleft = floorf(ua * 4.0f);
    uvright = floorf(ub * 4.0f);

    if (renderwall->portal != NULL) {
        *left = nca;
        *right = ncb;

        DrawWallColumns(false, true, CLIP_CEIL, CLIP_FLOOR);

        // If the height of the ceiling goes down, render top wall.
        if (renderwall->portal->ceiling < rendersector->ceiling) {
            heightceiling = rendereyeheight - rendersector->ceiling;
            heightfloor = rendereyeheight - renderwall->portal->ceiling;
            DrawWallColumns(true, false, CLIP_NONE, CLIP_CEIL);
        }

        // If the height of the floor goes up, render bottom wall.
        if (renderwall->portal->floor > rendersector->floor) {
            heightceiling = rendereyeheight - renderwall->portal->floor;
            heightfloor = rendereyeheight - rendersector->floor;
            DrawWallColumns(true, false, CLIP_FLOOR, CLIP_NONE);
        }

        return true;
    } else {
        DrawWallColumns(true, true, CLIP_NONE, CLIP_NONE);
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

void R_DrawSector(sector_t *sector, const vector_t *pos, float ang, float eyeheight) {
    // Set globals.
    U_VecCopy(&renderpos, pos);
    renderangle = ang;
    wallsine = sinf(-renderangle);
    wallcosine = cosf(-renderangle);
    flatsine = floorf(wallsine * SCRNDIST);
    flatcosine = floorf(wallcosine * SCRNDIST);
    renderbuf = playdate->graphics->getFrame();
    memset(renderminy, 0, sizeof(renderminy));
    memset(rendermaxy, LCD_ROWS, sizeof(rendermaxy));
    rendereyeheight = eyeheight;
    // Run recursive sector drawing.
    DrawSectorRecursive(sector, -200.0f, 200.0f);
    // Notify the system that we've drawn over the framebuffer.
    playdate->graphics->markUpdatedRows(0, LCD_ROWS - 1);
}
