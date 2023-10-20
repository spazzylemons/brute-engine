#include "r_local.h"
#include "r_sector.h"
#include "r_wall.h"

#include <math.h>

void R_DrawSector(sector_t *sector, float left, float right) {
    rendersector = sector;
    // Check each wall in the sector.
    for (size_t i = 0; i < sector->num_walls; i++) {
        const wall_t *wall = &sector->walls[i];
        // Bounds of wall.
        float nleft = left;
        float nright = right;
        // Check if portal should be drawn.
        bool draw_portal = R_DrawWall(wall, &nleft, &nright);
        if (draw_portal && fabsf(nleft - nright) >= 0.5f) {
            // If it should be drawn, draw it recursively using clipped bounds.
            R_DrawSector(wall->portal, nleft, nright);
            // Restore globals.
            rendersector = sector;
        }
    }
}
