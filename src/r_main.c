#include "r_draw.h"
#include "r_flat.h"
#include "r_local.h"
#include "r_main.h"
#include "r_sector.h"
#include "r_wall.h"

void R_RenderViewpoint(const actor_t *actor) {
    // Load the framebuffer.
    R_LoadFramebuffer();
    // Init state of each submodule.
    U_VecCopy(&renderpos, &actor->pos);
    R_InitWallGlobals(actor->angle, actor->zpos + 32.0f);
    R_InitFlatGlobals(actor->angle);
    // Draw the sector that the actor is in.
    R_DrawSector(actor->sector, -SCRNDIST, SCRNDIST);
    // Flush the framebuffer.
    R_FlushFramebuffer();
}
