#ifndef BRUTE_R_DRAW_H
#define BRUTE_R_DRAW_H

#include "m_defs.h"

// Draw a sector at the given position and angle.
void R_DrawSector(uint8_t *framebuffer, sector_t *sector, const vector_t *pos, float ang);

// Draw a top-down view of the map.
void R_DrawAutoMap(sector_t *sector, const vector_t *pos, float ang);

#endif
