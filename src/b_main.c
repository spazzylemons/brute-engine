#include "a_classes.h"
#include "g_menu.h"
#include "i_input.h"
#include "i_system.h"
#include "i_video.h"
#include "m_load.h"
#include "r_actor.h"
#include "r_draw.h"
#include "r_main.h"
#include "w_pack.h"
#include "z_memory.h"

static map_t *map = NULL;
static actor_t *player = NULL;

actor_t *fortesting;

void B_MainInit(void) {
    if (!W_Open("assets.bin")) {
        I_Error("Failed to open pack");
    }

    // Init modules.
    G_MenuInit();
    M_Init();
    R_LoadSprites();

    map = M_Load("map01");

    // Spawn the player.
    vector_t pos;
    pos.x = 0.0f;
    pos.y = 0.0f;
    player = A_ActorSpawn(&A_ClassPlayer, &pos, 0.0f, &map->scts[0]);

    // Spawn an empty object in front of the player.
    pos.y = 64.0f;
    fortesting = A_ActorSpawn(&A_ClassEmpty, &pos, 0.0f, &map->scts[0]);
}

void B_MainLoop(void) {
    R_LoadFramebuffer();
    if (!G_IsMenuOpen()) {
        A_ActorUpdate();
        // If menu button pressed, open menu.
        if (I_GetPressedButtons() & BTN_M) {
            G_MenuReset();
        }
    }
    // Draw player viewpoint.
    R_RenderViewpoint(player);
    if (G_IsMenuOpen()) {
        G_MenuShow();
    }
    R_FlushFramebuffer();

    I_DrawFPS();
}

void B_MainQuit(void) {
    // Clean up modules.
    A_ActorClear();
    G_MenuDeinit();
    M_Free(map);
    R_FreeSprites();
    W_Close();

#ifdef DEBUG_ALLOCATOR
    CheckLeaks();
#endif
}
