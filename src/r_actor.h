#ifndef BRUTE_R_ACTOR_H
#define BRUTE_R_ACTOR_H

/**
 * Routines for drawing actors.
 */

#include "a_actor.h"
#include "r_defs.h"

// Contains a pointer to a drawn wall and its X and Y bounds.
typedef struct {
    const wall_t *wall;   // Wall that this viswall represents.
    uint16_t minx;        // Left X coordinate of portal, inclusive.
    uint16_t maxx;        // Right X coordinate of portal, exclusive.
    const uint8_t *miny;  // Minimum Y clipping bounds, inclusive.
    const uint8_t *maxy;  // Maximum Y clipping bounds, exclusive.
} viswall_t;

// Load the sprites.
void R_LoadSprites(void);

// Unload the sprites.
void R_FreeSprites(void);

// Empty the viswall stack.
void R_ClearViswalls(void);

// Get a pointer to a viswall to write to, or NULL if out of space.
viswall_t *R_NewViswall(void);

// Draw actors.
void R_DrawActors(void);

#endif
