#ifndef BRUTE_R_DRAW_H
#define BRUTE_R_DRAW_H

/**
 * Low-level drawing routines.
 * Before calling, make sure that the bounds are correct and that all parameters
 * are set. These functions make no checks.
 */

#include <stdint.h>

// Load the current framebuffer. Call this before calling other routines in a frame.
void R_LoadFramebuffer(void);

// Flush the framebuffer. Call this after finished drawing a scene.
void R_FlushFramebuffer(void);

// Parameters for R_DrawColumn.
extern const uint8_t *dc_source; // Column to draw.
extern uint16_t       dc_height; // Height of column to draw.
extern uint32_t       dc_scale;  // Amount to stretch, 20.12 fixed point.
extern int32_t        dc_offset; // Offset of column texture.
extern uint16_t       dc_x;      // X coordinate to draw in.
extern uint8_t        dc_yh;     // Top Y coordinate of column, inclusive.
extern uint8_t        dc_yl;     // Bottom Y coordinate of column, exclusive.

// Draw a column top-down. Bounds are not checked.
void R_DrawColumn(void);

// Parameters for R_DrawSpan.
extern const uint8_t *ds_source; // Span to draw.
extern int32_t        ds_xstep;  // Amount to step X coordinate, 20.12 fixed point.
extern int32_t        ds_ystep;  // Amount to step Y coordinate, 20.12 fixed point.
extern int32_t        ds_xfrac;  // Starting X coordinate, 20.12 fixed point.
extern int32_t        ds_yfrac;  // Starting Y coordinate, 20.12 fixed point.
extern uint16_t       ds_x1;     // Left X coordinate of span, inclusive.
extern uint16_t       ds_x2;     // Right X coordinate of span, exclusive.
extern uint8_t        ds_y;      // Y coordinate to draw in.

// Draw a span left to right. Bounds are not checked.
void R_DrawSpan(void);

#endif
