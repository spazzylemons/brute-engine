#include "a_classes.h"
#include "i_video.h"
#include "m_load.h"
#include "r_main.h"

static map_t *map = NULL;
static actor_t *player = NULL;

void B_MainInit(void) {
    map = M_Load("map01");
    vector_t pos;
    pos.x = 0.0f;
    pos.y = 0.0f;
    player = A_ActorSpawn(&A_ClassPlayer, &pos, 0.0f, &map->scts[0]);
}

void B_MainLoop(void) {
    A_ActorUpdate();
    R_RenderViewpoint(player);
    I_DrawFPS();
}

void B_MainQuit(void) {
    A_ActorClear();
    M_Free(map);

#ifdef DEBUG_ALLOCATOR
    CheckLeaks();
#endif
}
