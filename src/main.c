#include "system.h"
#include "video.h"
#include "map/load.h"
#include "render/actor.h"
#include "render/draw.h"
#include "render/main.h"

void B_MainInit(void) {
    // Init modules.
    load_sprites();
}

void B_MainQuit(void) {
    free_sprites();
}
