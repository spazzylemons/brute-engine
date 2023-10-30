#ifndef BRUTE_R_FLAT_H
#define BRUTE_R_FLAT_H

/**
 * Flats are floors and ceilings. These rendering routines handle just that.
 */

#include "m_defs.h"

// Initialize globals used for flat rendering. Call once before drawing a scene.
void R_InitFlatGlobals(float angle);

void R_InitFlatBounds(void);

void R_UpdateFlatBounds(void);

// Draw a flat using the given floor or ceiling height and Y bounds.
void R_DrawFlat(const flat_t *flat, const uint8_t *miny, const uint8_t *maxy, int32_t height);

#endif
