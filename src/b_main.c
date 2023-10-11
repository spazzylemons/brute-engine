#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>

#include "pd_api.h"

#include "a_classes.h"
#include "m_load.h"
#include "m_map.h"
#include "r_draw.h"

PlaydateAPI *playdate;

static map_t *map = NULL;

static actor_t *player = NULL;
// static bool draw_3d = true;

#define PI 3.141592653589793f

static int update(void *userdata) {
    (void) userdata;

    if (player != NULL) {
        player->class->update(player);
        playdate->graphics->clear(kColorBlack);
        R_DrawSector(playdate->graphics->getFrame(), player->sector, &player->pos, player->angle);
    }

    playdate->system->drawFPS(0, 0);

    return 1;
}

#ifdef _WINDLL
__declspec(dllexport)
#endif
int eventHandler(PlaydateAPI *pd, PDSystemEvent event, uint32_t arg) {
    (void) arg;

    if (event == kEventInit) {
        playdate = pd;
        playdate->system->setUpdateCallback(update, NULL);
        map = M_Load("map01");
        if (map != NULL) {
            vector_t pos;
            pos.x = 0.0f;
            pos.y = 0.0f;
            player = A_ActorSpawn(&A_ClassPlayer, &pos, 0.0f, &map->scts[0]);
        }
    }
    
    return 0;
}
