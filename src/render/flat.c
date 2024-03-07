#include "video.h"
#include "render/draw.h"
#include "render/flat.h"
#include "render/local.h"

#include <math.h>
#include <string.h>

static fixed_t offx;       // X offset
static fixed_t offy;       // Y offset
static fixed_t flatsine;   // Sine angle value
static fixed_t flatcosine; // Cosine angle value

static fixed_t heightcos;
static fixed_t heightsin;

void R_InitFlatGlobals(float angle) {
    // Calculate the X and Y offsets.
    offx = float_to_fixed(renderpos.x) & ((0x40 << FRACBITS) - 1);
    offy = float_to_fixed(renderpos.y) & ((0x40 << FRACBITS) - 1);
    // Calculate the sine and cosine.
    flatsine = float_to_fixed(SCRNDIST * sinf(-angle));
    flatcosine = float_to_fixed(SCRNDIST * cosf(-angle));
}

static void DrawLine(uint8_t y, uint16_t x1, uint16_t x2) {
    int32_t den = (int32_t) y - 120;
    // To be careful, don't draw if we'd divide by zero.
    if (den != 0) {
        ds_x1 = x1;
        ds_x2 = x2;
        ds_y = y;
        ds_xstep = -heightcos / (den * SCRNDISTI);
        ds_ystep = -heightsin / (den * SCRNDISTI);
        ds_xfrac = (ds_xstep * ds_x1 + ((heightcos - heightsin) / den) - offx);
        ds_yfrac = (ds_ystep * ds_x1 + ((heightsin + heightcos) / den) + offy);
        R_DrawSpan();
    }
}

void R_DrawFlat(const flat_t *flat, const uint8_t *miny, const uint8_t *maxy, int32_t height) {
    static uint16_t spanstart[SCREENHEIGHT];

    // Calculate the height values.
    heightcos = fixed_mul(height, flatcosine);
    heightsin = fixed_mul(height, flatsine);
    // Set span source.
    ds_source = flat->data;

    uint16_t startx = sectorxmin;
    uint8_t t1, b1;
    for (;;) {
        t1 = miny[startx];
        b1 = maxy[startx];
        if (t1 < b1) {
            break;
        }
        if (++startx == sectorxmax) {
            return;
        }
    }
    for (uint8_t y = t1; y < b1; y++) {
        spanstart[y] = startx;
    }
    // Iterate over spans.
    for (uint16_t x = startx + 1; x < sectorxmax; x++) {
        uint8_t t2 = miny[x];
        uint8_t b2 = maxy[x];
        while (t1 < t2 && t1 < b1) {
            DrawLine(t1, spanstart[t1], x);
            t1++;
        }
        while (b1 > b2 && t1 < b1) {
            b1--;
            DrawLine(b1, spanstart[b1], x);
        }
        while (t1 > t2 && t1 < b1) {
            t1--;
            spanstart[t1] = x;
        }
        while (b1 < b2 && t1 < b1) {
            spanstart[b1] = x;
            b1++;
        }
        if (x + 1 < sectorxmax) {
            t1 = t2;
            b1 = b2;
        }
    }
    // Finish drawing spans that reach to the edge.
    for (uint8_t y = t1; y < b1; y++) {
        DrawLine(y, spanstart[y], sectorxmax);
    }
}