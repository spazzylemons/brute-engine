#ifndef BRUTE_I_VIDEO_H
#define BRUTE_I_VIDEO_H

/**
 * Video interfacing functions.
 */

#include <stdint.h>

#define SCREENWIDTH  400
#define SCREENHEIGHT 240

// Get the current framebuffer.
uint8_t *__attribute__((aligned(4))) I_GetFramebuffer(void);

// Mark framebuffer as updated to invalidate caches.
void I_MarkFramebufferDirty(void);

// Draw the FPS in the top-left corner.
void I_DrawFPS(void);

#endif
