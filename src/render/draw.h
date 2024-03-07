#ifndef BRUTE_R_DRAW_H
#define BRUTE_R_DRAW_H

/**
 * Low-level drawing routines.
 * Before calling, make sure that the bounds are correct and that all parameters
 * are set. These functions make no checks.
 */

#include "render/fixed.h"

// Load the current framebuffer. Call this before calling other routines in a frame.
void R_LoadFramebuffer(void);

// Flush the framebuffer. Call this after finished drawing a scene.
void R_FlushFramebuffer(void);

// Parameters for R_DrawColumn.
extern const uint8_t *dc_source; // Column to draw.
extern uint16_t       dc_height; // Height of column to draw.
extern fixed_t        dc_scale;  // Amount to stretch.
extern fixed_t        dc_offset; // Offset of column texture.
extern uint16_t       dc_x;      // X coordinate to draw in.
extern uint8_t        dc_yh;     // Top Y coordinate of column, inclusive.
extern uint8_t        dc_yl;     // Bottom Y coordinate of column, exclusive.

// Draw a column top-down. Bounds are not checked.
void R_DrawColumn(void);

// Parameters for R_DrawSpan.
extern const uint8_t *ds_source; // Span to draw.
extern fixed_t        ds_xstep;  // Amount to step X coordinate.
extern fixed_t        ds_ystep;  // Amount to step Y coordinate.
extern fixed_t        ds_xfrac;  // Starting X coordinate.
extern fixed_t        ds_yfrac;  // Starting Y coordinate.
extern uint16_t       ds_x1;     // Left X coordinate of span, inclusive.
extern uint16_t       ds_x2;     // Right X coordinate of span, exclusive.
extern uint8_t        ds_y;      // Y coordinate to draw in.

// Draw a span left to right. Bounds are not checked.
void R_DrawSpan(void);

// Parameters for R_Blit.
extern const uint8_t *blit_source; // Bits to blit. Must be in interleaved format.
extern uint8_t        blit_length; // Number of bytes to blit.
extern uint16_t       blit_x;      // Starting X position.
extern uint8_t        blit_y;      // Y position.

// Blit a row of bits to the framebuffer. Bounds are not checked.
// Bits should not be blitted partially off-screen.
void R_Blit(void);

#endif
