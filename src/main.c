#include <stdio.h>
#include <stdlib.h>

#include "pd_api.h"

#include "map/load.h"

PlaydateAPI *playdate;

static int sector_stack_position = 0;
static int sector_stack_top = 1;
static map_t *map = NULL;
static sector_t *sector_stack[30];

static int update(void *userdata) {
    (void) userdata;

    if (map != NULL) {
        if (sector_stack_position != sector_stack_top) {
            sector_t *sector = sector_stack[sector_stack_position++];
            for (int i = 0; i < sector->num_walls; i++) {
                wall_t *wall = &sector->walls[i];
                if (wall->portal != NULL) {
                    for (int j = 0; j < sector_stack_top; j++) {
                        if (sector_stack[j] == wall->portal) {
                            goto no_add;
                        }
                    }
                    sector_stack[sector_stack_top++] = wall->portal;
                no_add:
                    ;
                }
                int x1 = ((wall->v1->x >> FRACBITS) >> 2) + 200;
                int y1 = ((wall->v1->y >> FRACBITS) >> 2) + 120;
                int x2 = ((wall->v2->x >> FRACBITS) >> 2) + 200;
                int y2 = ((wall->v2->y >> FRACBITS) >> 2) + 120;
                playdate->graphics->drawLine(x1, y1, x2, y2, 1, kColorBlack);
            }
        }
    }

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
        sector_stack[0] = &map->scts[0];
    }
    
    return 0;
}
