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
static int32_t rendereyeheight;

// Enum for how to clip sector bounds.
typedef enum {
    CLIP_NONE,  // Perform no clipping.
    CLIP_CEIL,  // Clip the ceiling Y bounds.
    CLIP_FLOOR, // Clip the floor Y bounds.
} cliptype_t;

// Buffer for storing Y pixel coordinate of portal top.
static uint8_t renderminy[LCD_COLUMNS];
// Buffer for storing Y pixel coordinate of portal bottom.
static uint8_t rendermaxy[LCD_COLUMNS];
// Buffer for storing previous Y pixel top, for flat drawing.
static uint8_t prevminy[LCD_COLUMNS];
// Buffer for storing previous Y pixel bottom, for flat drawing.
static uint8_t prevmaxy[LCD_COLUMNS];

static uint16_t flatleft[LCD_ROWS];
static uint16_t flatright[LCD_ROWS];
static uint8_t flattop;
static uint8_t flatbottom;

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
            if (val > rendermaxy[x]) {
                val = rendermaxy[x];
            }
            if (val > renderminy[x]) {
                renderminy[x] = val;
            }
            break;
        case CLIP_FLOOR:
            if (val < renderminy[x]) {
                val = renderminy[x];
            }
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

static const uint8_t shades[18][4] = {
    DitherPattern(0x0, 0x0, 0x0, 0x0), // Transparent?
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
    // TODO buffer overflow can occur if we have a texture that accesses higher
    // palette indices. We should check for this.
};

// Stride of rows in framebuffer.
#define ROWSTRIDE 52

// Get the min and max X bounds.
static void FindXBounds(const uint8_t *miny, const uint8_t *maxy) {
    // Where are the min and max Y coordinates?
    flattop = LCD_ROWS;
    flatbottom = 0;
    for (uint16_t x = renderxmin; x < renderxmax; x++) {
        if (miny[x] < flattop) flattop = miny[x];
        if (maxy[x] > flatbottom) flatbottom = maxy[x];
    }
    // Go through these rows and find the bounds.
    for (uint8_t y = flattop; y < flatbottom; y++) {
        int32_t x;
        for (x = renderxmin; x < renderxmax; x++) {
            if (y >= miny[x] && y < maxy[x]) {
                break;
            }
        }
        flatleft[y] = x;
        for (x = renderxmax - 1; x >= renderxmin; x--) {
            if (y >= miny[x] && y < maxy[x]) {
                break;
            }
        }
        flatright[y] = x + 1;
    }
}

// Draw a floor or ceiling.
static void NewDrawFlat(const uint8_t *miny, const uint8_t *maxy, int32_t height) {
    FindXBounds(miny, maxy);
    int32_t offx = floorf(renderpos.x * 256.0f);
    int32_t offy = floorf(renderpos.y * 256.0f);
    int32_t heightcos = height * flatcosine;
    int32_t heightsin = height * flatsine;
    for (int32_t y = flattop; y < flatbottom; y++) {
        int32_t den = y - 120;
        // To be careful, don't draw if we'd divide by zero.
        if (den != 0) {
            uint16_t startx = flatleft[y];
            uint8_t *framebuffer = renderbuf + (ROWSTRIDE * y) + (startx >> 3);
            uint8_t mask = 1 << (7 - (startx & 7));
            int32_t fracstepx = -heightcos / (den * SCRNDISTI);
            int32_t fracstepy = -heightsin / (den * SCRNDISTI);
            int32_t fracx = (fracstepx * startx + ((heightcos - heightsin) / den) - offx);
            int32_t fracy = (fracstepy * startx + ((heightsin + heightcos) / den) + offy);
            for (uint16_t x = startx; x < flatright[y]; x++) {
                uint16_t newx = (fracx >> 8) & 63;
                uint16_t newy = (fracy >> 8) & 63;
                uint16_t flatindex = newx + (newy << 6);
                uint8_t pixel = rendersector->floorflat->data[flatindex];
                if (shades[pixel][y & 3] & mask) {
                    *framebuffer |= mask;
                } else {
                    *framebuffer &= ~mask;
                }
                fracx += fracstepx;
                fracy += fracstepy;
                mask >>= 1;
                if (mask == 0) {
                    mask = 0x80;
                    ++framebuffer;
                }
            }
        }
    }
}

static void DrawFlats(void) {
    NewDrawFlat(prevminy, renderminy, heightceiling);
    NewDrawFlat(rendermaxy, prevmaxy, heightfloor);
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
    for (int32_t y = y1; y < y2; y++) {
        int32_t whichy = (frac >> 12) & scale;
        uint16_t pixel = source[whichy];
        if (shades[pixel][y & 3] & renderxmask) {
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
                &patch->data[whichx * patch->height],
                patch->height,
                heightfloor - heightceiling,
                x + x1,
                ay1,
                ay2
            );
        }
        prevminy[x + x1] = renderminy[x + x1];
        prevmaxy[x + x1] = rendermaxy[x + x1];
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

        DrawWallColumns(false, CLIP_CEIL, CLIP_FLOOR);
        DrawFlats();

        // If the height of the ceiling goes down, render top wall.
        if (renderwall->portal->ceiling < rendersector->ceiling) {
            heightceiling = rendereyeheight - rendersector->ceiling;
            heightfloor = rendereyeheight - renderwall->portal->ceiling;
            DrawWallColumns(true, CLIP_NONE, CLIP_CEIL);
        }

        // If the height of the floor goes up, render bottom wall.
        if (renderwall->portal->floor > rendersector->floor) {
            heightceiling = rendereyeheight - renderwall->portal->floor;
            heightfloor = rendereyeheight - rendersector->floor;
            DrawWallColumns(true, CLIP_FLOOR, CLIP_NONE);
        }

        return true;
    } else {
        DrawWallColumns(true, CLIP_CEIL, CLIP_FLOOR);
        DrawFlats();
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
    flatsine = (int32_t) floorf(wallsine * SCRNDIST) << 8;
    flatcosine = (int32_t) floorf(wallcosine * SCRNDIST) << 8;
    renderbuf = playdate->graphics->getFrame();
    memset(renderminy, 0, sizeof(renderminy));
    memset(rendermaxy, LCD_ROWS, sizeof(rendermaxy));
    memset(prevminy, 0, sizeof(prevminy));
    memset(prevmaxy, LCD_ROWS, sizeof(prevmaxy));
    rendereyeheight = floorf(eyeheight);
    // Run recursive sector drawing.
    DrawSectorRecursive(sector, -200.0f, 200.0f);
    // Notify the system that we've drawn over the framebuffer.
    playdate->graphics->markUpdatedRows(0, LCD_ROWS - 1);
}
