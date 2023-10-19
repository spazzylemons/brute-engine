#include "r_drawing.h"

#define ROWSTRIDE 52

extern uint8_t *__attribute__((aligned(4))) renderbuf;

const uint8_t *dc_source;
uint16_t       dc_height;
uint32_t       dc_scale;
int32_t        dc_offset;
uint16_t       dc_x;
uint8_t        dc_yh;
uint8_t        dc_yl;

const uint8_t *ds_source;
int32_t        ds_xstep;
int32_t        ds_ystep;
int32_t        ds_xfrac;
int32_t        ds_yfrac;
uint16_t       ds_x1;
uint16_t       ds_x2;
uint8_t        ds_y;

#define DitherPattern(a, b, c, d) { a * 0x11, b * 0x11, c * 0x11, d * 0x11 }

static const uint8_t drawshades[18][4] = {
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

// Plot a pixel.
static inline void PlotPixel(uint8_t *framebuffer, uint8_t shade, uint8_t mask) {
    uint8_t value = *framebuffer;
    if ((value ^ shade) & mask) {
        value ^= mask;
        *framebuffer = value;
    }
}

void R_DrawColumn(void) {
    uint16_t yh = dc_yh;
    uint16_t yl = dc_yl;
    // Framebuffer and mask to draw to.
    uint8_t *framebuffer = &renderbuf[(dc_x >> 3) + (ROWSTRIDE * yh)];
    uint8_t xmask = 1 << (7 - (dc_x & 7));
    const uint8_t *source = dc_source;
    // For speed, calculate fixed-point accumulator instead of repeated multiply and divide.
    int32_t fracstep = dc_scale;
    int32_t frac = fracstep * (yh - dc_offset);
    // Convert scale to mask.
    uint8_t mask = dc_height - 1;
    for (uint8_t y = yh; y < yl; y++) {
        // Plot pixel.
        uint16_t pixel = source[(frac >> 12) & mask];
        PlotPixel(framebuffer, drawshades[pixel][y & 3], xmask);
        // Advance fractional step.
        frac = (frac + fracstep) & ((mask << 12) | 0xfff);
        // Move to next row to copy to.
        framebuffer += ROWSTRIDE;
    }
}

void R_DrawSpan(void) {
    uint16_t x1 = ds_x1;
    uint16_t x2 = ds_x2;
    uint8_t y = ds_y;
    const uint8_t *source = ds_source;
    // Framebuffer and mask to draw to.
    uint8_t *framebuffer = &renderbuf[(x1 >> 3) + (ROWSTRIDE * y)];
    uint8_t xmask = 1 << (7 - (x1 & 7));
    // Copy variables.
    int32_t fracstepx = ds_xstep;
    int32_t fracstepy = ds_ystep;
    int32_t fracx = ds_xfrac;
    int32_t fracy = ds_yfrac;
    y &= 3;
    for (uint16_t x = x1; x < x2; x++) {
        // Calculate index.
        uint8_t newx = (fracx >> 12) & 63;
        uint8_t newy = (fracy >> 12) & 63;
        uint16_t flatindex = newx + (newy << 6);
        // Plot pixel.
        uint8_t pixel = source[flatindex];
        PlotPixel(framebuffer, drawshades[pixel][y], xmask);
        // Advance fractional steps.
        fracx += fracstepx;
        fracy += fracstepy;
        // Move to next pixel.
        xmask >>= 1;
        if (xmask == 0) {
            xmask = 0x80;
            ++framebuffer;
        }
    }
}
