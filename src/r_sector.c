#include "r_local.h"
#include "r_sector.h"
#include "r_wall.h"

#include <math.h>

#define MAXSECTORDEPTH 32

typedef struct {
    const sector_t *sector;
    float left, right;
} sectorstack_t;

void R_DrawSector(sector_t *sector, float left, float right) {
    // Stack of sector data.
    static sectorstack_t sectorstack[MAXSECTORDEPTH];

    // Initialize stack.
    uint8_t depth = 1;
    sectorstack[0].sector = sector;
    sectorstack[0].left = left;
    sectorstack[0].right = right;

    // Recursive stack drawing.
    do {
        // Pop from stack.
        --depth;
        rendersector = sectorstack[depth].sector;
        left = sectorstack[depth].left;
        right = sectorstack[depth].right;
        // Check each wall in the sector.
        for (size_t i = 0; i < rendersector->num_walls; i++) {
            const wall_t *wall = &rendersector->walls[i];
            // Bounds of wall.
            float nleft = left;
            float nright = right;
            // Check if portal should be drawn.
            if (R_DrawWall(wall, &nleft, &nright)) {
                if (__builtin_expect(depth < MAXSECTORDEPTH, 0)) {
                    sectorstack[depth].sector = wall->portal;
                    sectorstack[depth].left = nleft;
                    sectorstack[depth].right = nright;
                    depth++;
                }
            }
        }
    } while (depth != 0);
}
