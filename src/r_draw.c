#include "i_video.h"
#include "r_draw.h"

// Number of bytes in a framebuffer row.
#define ROWSTRIDE 52

// Mask for iterating steps.
#define STEPMASK(_mask_) (((_mask_) << FRACBITS) | ((1 << FRACBITS) - 1))

// Mask for iterating flat steps.
#define FLATMASK STEPMASK(63)

// Framebuffer.
static uint8_t *__attribute__((aligned(4))) renderbuf;

const uint8_t *dc_source;
uint16_t       dc_height;
fixed_t        dc_scale;
fixed_t        dc_offset;
uint16_t       dc_x;
uint8_t        dc_yh;
uint8_t        dc_yl;

const uint8_t *ds_source;
fixed_t        ds_xstep;
fixed_t        ds_ystep;
fixed_t        ds_xfrac;
fixed_t        ds_yfrac;
uint16_t       ds_x1;
uint16_t       ds_x2;
uint8_t        ds_y;

#define DitherPattern(a, b, c, d) { a * 0x11, b * 0x11, c * 0x11, d * 0x11 }

static const uint8_t drawshades[17][4] = {
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

void R_LoadFramebuffer(void) {
    renderbuf = I_GetFramebuffer();
}

void R_FlushFramebuffer(void) {
    I_MarkFramebufferDirty();
}

void R_DrawColumn(void) {
    uint8_t yh = dc_yh;
    uint8_t yl = dc_yl;
    // Framebuffer and mask to draw to.
    uint8_t *framebuffer = &renderbuf[(dc_x >> 3) + (ROWSTRIDE * yh)];
    uint8_t xmask = 1 << (7 - (dc_x & 7));
    const uint8_t *source = dc_source;
    // For speed, use fixed-point accumulator instead of repeated multiply and divide.
    fixed_t fracstep = dc_scale;
    // Convert scale to mask.
    fixed_t mask = STEPMASK(dc_height - 1);
    fixed_t frac = (fracstep * (yh - (SCREENHEIGHT >> 1)) + dc_offset) & mask;
    for (uint8_t y = yh; y < yl; y++) {
        // Plot pixel.
        uint8_t pixel = source[frac >> FRACBITS];
        PlotPixel(framebuffer, drawshades[pixel][y & 3], xmask);
        // Advance fractional step.
        frac = (frac + fracstep) & mask;
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
    fixed_t fracstepx = ds_xstep;
    fixed_t fracstepy = ds_ystep;
    fixed_t fracx = ds_xfrac & FLATMASK;
    fixed_t fracy = ds_yfrac & FLATMASK;
    y &= 3;
    for (uint16_t x = x1; x < x2; x++) {
        // Calculate index.
        uint8_t newx = fracx >> FRACBITS;
        uint8_t newy = fracy >> FRACBITS;
        uint16_t index = newx | (newy << 6);
        // Plot pixel.
        uint8_t pixel = source[index];
        PlotPixel(framebuffer, drawshades[pixel][y], xmask);
        // Advance fractional steps.
        fracx = (fracx + fracstepx) & FLATMASK;
        fracy = (fracy + fracstepy) & FLATMASK;
        // Move to next pixel.
        xmask >>= 1;
        if (xmask == 0) {
            xmask = 0x80;
            ++framebuffer;
        }
    }
}
