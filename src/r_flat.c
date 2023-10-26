#include "i_video.h"
#include "r_draw.h"
#include "r_flat.h"
#include "r_local.h"

#include <math.h>

static fixed_t offx;       // X offset
static fixed_t offy;       // Y offset
static fixed_t flatsine;   // Sine angle value
static fixed_t flatcosine; // Cosine angle value

static uint16_t flatleft[SCREENHEIGHT];
static uint16_t flatright[SCREENHEIGHT];
static uint8_t flattop;
static uint8_t flatbottom;

void R_InitFlatGlobals(float angle) {
    // Calculate the X and Y offsets.
    offx = R_FloatToFixed(renderpos.x) & ((0x40 << FRACBITS) - 1);
    offy = R_FloatToFixed(renderpos.y) & ((0x40 << FRACBITS) - 1);
    // Calculate the sine and cosine.
    flatsine = R_FloatToFixed(SCRNDIST * sinf(-angle));
    flatcosine = R_FloatToFixed(SCRNDIST * cosf(-angle));
}

// Get the min and max X bounds.
static void FindXBounds(const uint8_t *miny, const uint8_t *maxy) {
    // Where are the min and max Y coordinates?
    flattop = SCREENHEIGHT;
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

void R_DrawFlat(const flat_t *flat, const uint8_t *miny, const uint8_t *maxy, int32_t height) {
    FindXBounds(miny, maxy);
    fixed_t heightcos = R_FixedMul(height, flatcosine);
    fixed_t heightsin = R_FixedMul(height, flatsine);
    // Set span source.
    ds_source = flat->data;
    // Draw spans.
    for (uint8_t y = flattop; y < flatbottom; y++) {
        int32_t den = y - 120;
        // To be careful, don't draw if we'd divide by zero.
        if (den != 0) {
            ds_x1 = flatleft[y];
            ds_x2 = flatright[y];
            ds_y = y;
            ds_xstep = -heightcos / (den * SCRNDISTI);
            ds_ystep = -heightsin / (den * SCRNDISTI);
            ds_xfrac = (ds_xstep * ds_x1 + ((heightcos - heightsin) / den) - offx);
            ds_yfrac = (ds_ystep * ds_x1 + ((heightsin + heightcos) / den) + offy);
            R_DrawSpan();
        }
    }
}