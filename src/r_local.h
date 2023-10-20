#ifndef BRUTE_R_LOCAL_H
#define BRUTE_R_LOCAL_H

/**
 * Global variables and constants used exclusively by the render module.
 */

#include "m_defs.h"

// Half of screen width.
#define SCRNDIST 200.0f

// Half of screen width as integer.
#define SCRNDISTI 200

extern uint16_t renderxmin; // Left X coordinate of wall rendering, inclusive.
extern uint16_t renderxmax; // Right X coordinate of wall rendering, exclusive.

extern const sector_t *rendersector; // Sector being rendered.

extern vector_t renderpos; // Position scene is rendered at.

#endif
