#include "r_actor.h"
#include "r_draw.h"
#include "r_flat.h"
#include "r_local.h"
#include "r_sector.h"
#include "r_wall.h"

#include <math.h>

typedef struct {
    const sector_t *sector;
    float left, right;
} sectorstack_t;

// Maximum number of sectors on the stack.
#define MAXSECTORDEPTH 32

// Size of clipping buffer. When exhausted, no more sprites are drawn.
#define CLIPBUFSIZE 131072

// A buffer of clipping bounds. Used to clip sprites.
static uint8_t clipbuffer[CLIPBUFSIZE];
// Number of bytes allocated for clipbuffer.
static size_t numclipbytes;

void R_DrawSector(sector_t *sector, uint16_t left, uint16_t right) {
    // Stack of sector data.
    static sectorstack_t sectorstack[MAXSECTORDEPTH];

    numclipbytes = 0;

    R_ClearViswalls();

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
        sectorxmin = sectorstack[depth].left;
        sectorxmax = sectorstack[depth].right;
        uint16_t sectorsize = sectorxmax - sectorxmin;
        R_WallSectorHeight();
        R_WallYBoundsUpdate();
        // Check each wall in the sector.
        // Get pointers to clipping bounds.
        uint8_t *clipbuf = NULL;
        if (numclipbytes + (sectorsize * 2) <= CLIPBUFSIZE) {
            // Allocate clip buffer.
            clipbuf = &clipbuffer[numclipbytes];
            numclipbytes += sectorsize * 2;
        }
        for (size_t i = 0; i < rendersector->num_walls; i++) {
            const wall_t *wall = &rendersector->walls[i];
            // Bounds of wall.
            uint16_t nleft, nright;
            // Draw wall if possible.
            if (R_DrawWall(wall, &nleft, &nright)) {
                // If a portal, add to stack.
                if (wall->portal != NULL) {
                    if (__builtin_expect(depth < MAXSECTORDEPTH, 0)) {
                        sectorstack[depth].sector = wall->portal;
                        sectorstack[depth].left = nleft;
                        sectorstack[depth].right = nright;
                        depth++;
                    }
                }
                // Allocate and build viswall if the wall's drawn.
                if (clipbuf != NULL) {
                    viswall_t *viswall = R_NewViswall();
                    if (viswall != NULL) {
                        viswall->wall = wall;
                        viswall->minx = wallminx;
                        viswall->maxx = wallmaxx;
                        viswall->miny = &clipbuf[wallminx - sectorxmin];
                        viswall->maxy = viswall->miny + sectorsize;
                    }
                }
            }
        }
        // If we have clip buffer, fill it with the new clip bounds.
        if (clipbuf != NULL) {
            R_CopyClipBounds(clipbuf);
        }
        // Draw the floor and ceiling.
        // Note for later: It's bugged. Oops!
        R_DrawWallFlats();
    } while (depth != 0);
}
