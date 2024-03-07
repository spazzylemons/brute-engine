#include "video.h"
#include "system.h"
#include "render/actor.h"
#include "render/draw.h"
#include "render/flat.h"
#include "render/local.h"
#include "render/main.h"
#include "render/sector.h"
#include "render/wall.h"

#include <math.h>

#define TAU 6.2831853f

// Calculate view bobbing.
static float ViewBobbing(const actor_t *actor) {
    // Calculate where in animation we are.
    // TODO don't rely on system time, use in-game tics instead.
    float animangle = (playdate->system->getCurrentTimeMilliseconds() & 511) * (TAU / 512.0f);
    // Figure out the intensity of view bobbing.
    float mag = U_VecLenSq(&actor->vel) * 0.1f;
    return cosf(animangle) * mag;
}

void render_viewpoint(const actor_t *actor) {
    // Init state of each submodule.
    U_VecCopy(&renderpos, &actor->pos);    
    float eyeheight = actor->zpos;
    eyeheight += 32.0f;
    eyeheight += ViewBobbing(actor);
    R_InitWallGlobals(actor->angle, eyeheight);
    R_InitFlatGlobals(actor->angle);
    // Draw the sector that the actor is in.
    R_DrawSector(actor->sector, 0, SCREENWIDTH);
    // Draw actors on top of the level geometry.
    R_DrawActors();
}
