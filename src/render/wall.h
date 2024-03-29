#ifndef BRUTE_R_WALL_H
#define BRUTE_R_WALL_H

/**
 * Wall drawing routines.
 */

#include "map/defs.h"

#include <stdbool.h>

// Initialize globals used for wall rendering. Call once before drawing a scene.
void R_InitWallGlobals(float angle, float eyeheight);

void R_DrawWallFlats(void);

void R_WallSectorHeight(void);

void R_WallYBoundsUpdate(void);

void R_CopyClipBounds(uint8_t *buf);

void R_RotatePoint(vector_t *v);

// Draw a wall. Bounds to draw the wall should be stored in the pointed-to floats.
// If the wall is a portal and its sector should be drawn, the bounds of the
// portal are written in the given pointers and the function returns true.
// Also returns true if the wall is not a portal, as long as it is drawn.
bool R_DrawWall(const wall_t *wall, uint16_t *left, uint16_t *right);

#endif
