#ifndef BRUTE_R_WALL_H
#define BRUTE_R_WALL_H

/**
 * Wall drawing routines.
 */

#include "m_defs.h"

#include <stdbool.h>

// Initialize globals used for wall rendering. Call once before drawing a scene.
void R_InitWallGlobals(float angle, float eyeheight);

// Draw a wall. Bounds to draw the wall should be stored in the pointed-to floats.
// If the wall is a portal and its sector should be drawn, the bounds of the
// portal are written in the given pointers and the function returns true.
bool R_DrawWall(const wall_t *wall, float *left, float *right);

#endif
