#include "b_core.h"
#include "r_draw.h"
#include "r_flat.h"
#include "r_local.h"

static int32_t offx;       // 20.12 fixed point X offset
static int32_t offy;       // 20.12 fixed point Y offset
static int32_t flatsine;   // 20.12 fixed point sine angle value
static int32_t flatcosine; // 20.12 fixed point cosine angle value

static uint16_t flatleft[LCD_ROWS];
static uint16_t flatright[LCD_ROWS];
static uint8_t flattop;
static uint8_t flatbottom;

void R_InitFlatGlobals(float angle) {
    // Calculate the X and Y offsets.
    offx = (int32_t) floorf((fmodf(renderpos.x, 64.0f) + 64.0f) * 4096.0f) & 0x3ffff;
    offy = (int32_t) floorf((fmodf(renderpos.y, 64.0f) + 64.0f) * 4096.0f) & 0x3ffff;
    // Calculate the sine and cosine.
    flatsine = (int32_t) floorf(sinf(-angle) * SCRNDISTI) << 12;
    flatcosine = (int32_t) floorf(cosf(-angle) * SCRNDISTI) << 12;
}

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

void R_DrawFlat(const flat_t *flat, const uint8_t *miny, const uint8_t *maxy, int32_t height) {
    FindXBounds(miny, maxy);
    int32_t heightcos = height * flatcosine;
    int32_t heightsin = height * flatsine;
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