#include "i_video.h"
#include "i_time.h"
#include "r_draw.h"
#include "r_flat.h"
#include "r_local.h"
#include "r_main.h"
#include "r_sector.h"
#include "r_wall.h"
#include "u_math.h"

#include <math.h>

// Calculate view bobbing.
static float ViewBobbing(const actor_t *actor) {
    // Calculate where in animation we are.
    float animangle = (I_GetMillis() & 511) * (TAU / 512.0f);
    // Figure out the intensity of view bobbing.
    float mag = U_VecLenSq(&actor->vel) * 0.1f;
    return cosf(animangle) * mag;
}

void R_RenderViewpoint(const actor_t *actor) {
    // Load the framebuffer.
    R_LoadFramebuffer();
    // Init state of each submodule.
    U_VecCopy(&renderpos, &actor->pos);    
    float eyeheight = actor->zpos;
    eyeheight += 32.0f;
    eyeheight += ViewBobbing(actor);
    R_InitWallGlobals(actor->angle, eyeheight);
    R_InitFlatGlobals(actor->angle);
    // Draw the sector that the actor is in.
    R_DrawSector(actor->sector, 0, SCREENWIDTH);
    // Flush the framebuffer.
    R_FlushFramebuffer();
}
