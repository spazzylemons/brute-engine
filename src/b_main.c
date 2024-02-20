#include "i_input.h"
#include "i_system.h"
#include "i_video.h"
#include "m_load.h"
#include "r_actor.h"
#include "r_draw.h"
#include "r_main.h"
#include "z_memory.h"

static map_t *map = NULL;

// TODO!
map_t *currentMap;

void B_MainInit(void) {
    // Init modules.
    R_LoadSprites();
    map = M_Load("map01");
    currentMap = map;
}

void B_MainQuit(void) {
    // Clean up modules.
    M_Free(map);
    R_FreeSprites();

#ifdef DEBUG_ALLOCATOR
    CheckLeaks();
#endif
}
