#ifndef BRUTE_G_FONT_H
#define BRUTE_G_FONT_H

/**
 * Font system.
 */

#include "g_bob.h"

typedef struct {
    // 96 bobs for each ASCII codepoint. A codepoint may be NULL if not present.
    bob_t *glyphs[96];
} font_t;

// Load a font.
font_t *G_LoadFont(uint32_t branchnum);

// Free a font.
void G_FreeFont(font_t *font);

// Print some text using a font.
void G_PrintFont(const font_t *font, const char *text, uint16_t x, uint8_t y);

#endif
