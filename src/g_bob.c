#include "g_bob.h"
#include "i_video.h"
#include "r_draw.h"
#include "u_error.h"
#include "w_pack.h"
#include "z_memory.h"

#include <string.h>

typedef struct __attribute__((__packed__)) {
    uint16_t width;
    uint16_t height;
    uint8_t data[0];
} file_bob_t;

bob_t *G_LoadBob(uint32_t lumpnum) {
    // Read lump.
    size_t size;
    file_bob_t *fbob = W_ReadLump(lumpnum, &size);
    // Check data.
    if (fbob->width == 0 || fbob->height == 0) {
        Error("Empty bob");
    }
    size_t datasize = (((fbob->width + 7) >> 3) << 1) * fbob->height;
    if (size < datasize + sizeof(file_bob_t)) {
        Error("Bob missing data");
    }
    // Convert.
    bob_t *bob = Allocate(sizeof(bob_t) + datasize);
    bob->width = fbob->width;
    bob->height = fbob->height;
    memcpy(bob->data, fbob->data, datasize);
    // Free file data.
    Deallocate(fbob);
    // Return bob.
    return bob;
}

void G_DrawBob(const bob_t *bob, int32_t x, int32_t y) {
    // Horizontal bounds check.
    if (x < 0 || x + bob->width > SCREENWIDTH) {
        return;
    }
    uint16_t stride = ((bob->width + 7) >> 3) << 1;
    // Vertical bounds checks.
    if (y >= SCREENHEIGHT) {
        return;
    }
    const uint8_t *data = bob->data;
    const uint8_t *dataend = data + stride * bob->height;
    if (y < 0) {
        data += stride * -y;
        y = 0;
    }
    if (y + bob->height > SCREENHEIGHT) {
        // TODO!
        return;
    }
    if (data > dataend) {
        return;
    }
    // Draw rows.
    while (data != dataend) {
        blit_source = data;
        blit_length = stride >> 1;
        blit_x = x;
        blit_y = y;
        R_Blit();
        ++y;
        data += stride;
    }
}
