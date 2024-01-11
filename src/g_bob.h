#ifndef BRUTE_G_DEFS_H
#define BRUTE_G_DEFS_H

/**
 * GUI blitter objects.
 */

#include <stdint.h>

// Blitter object. "Sprite" is already used for 3D sprites, so I'm calling them
// bobs courtesy of the Amiga scene.
typedef struct {
    // Width of graphic in pixels.
    uint16_t width;
    // Height of graphic in pixels.
    uint16_t height;
    // Interleaved blitting data.
    uint8_t data[0];
} bob_t;

// Load a bob from a lump.
bob_t *G_LoadBob(uint32_t lumpnum);

// Draw a bob. Does not draw if sprite is partially off-screen horizontally.
// Will draw if sprite is partially off-screen vertically.
void G_DrawBob(const bob_t *bob, int32_t x, int32_t y);

#endif
