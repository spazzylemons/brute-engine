#include "g_font.h"
#include "u_error.h"
#include "w_pack.h"
#include "z_memory.h"

#include <string.h>

font_t *G_LoadFont(uint32_t branchnum) {
    font_t *result = Allocate(sizeof(font_t));
    memset(result, 0, sizeof(font_t));

    branchiter_t iter;
    W_IterInit(&iter, branchnum);
    uint32_t glyphlump;

    char name[8];
    while ((glyphlump = W_IterNext(&iter, name))) {
        if (name[0] >= 0x20 && name[0] <= 0x7f) {
            uint8_t index = name[0] - 0x20;
            if (result->glyphs[index] == NULL) {
                result->glyphs[index] = G_LoadBob(glyphlump);
            }
        }
    }

    return result;
}

void G_FreeFont(font_t *font) {
    for (uint8_t i = 0; i < 96; i++) {
        if (font->glyphs[i] != NULL) {
            Deallocate(font->glyphs[i]);
        }
    }
    Deallocate(font);
}

void G_PrintFont(const font_t *font, const char *text, uint16_t x, uint8_t y) {
    char c;
    while ((c = *text++)) {
        const bob_t *glyph = NULL;
        if (c >= 0x20 && c <= 0x7f) {
            // Try loading ASCII glyph.
            glyph = font->glyphs[(uint8_t) c - 0x20];
        }
        if (glyph == NULL) {
            // Use '?' for missing glyphs.
            glyph = font->glyphs['?' - 0x20];
        }
        if (glyph != NULL) {
            G_DrawBob(glyph, x, y);
            x += glyph->width;
        }
    }
}
