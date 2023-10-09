#include <stdio.h>
#include <stdlib.h>

#include "pd_api.h"

#include "map/load.h"

#include "m_trig.h"
#include "r_draw.h"

PlaydateAPI *playdate;

static int sector_stack_position = 0;
static int sector_stack_top = 1;
static map_t *map = NULL;
static sector_t *sector_stack[30];

static angle_t ang = DEG_0;
static vertex_t pos = { 0, 0 };

static int update(void *userdata) {
    (void) userdata;

    PDButtons held;
    playdate->system->getButtonState(&held, NULL, NULL);
    if (held & kButtonLeft) {
        ang -= 200;
    }
    if (held & kButtonRight) {
        ang += 200;
    }
    vertex_t delta;
    if (held & kButtonUp) {
        delta.x = -M_Sine(DEG_0 - ang) * 2.0f;
        delta.y = M_Cosine(DEG_0 - ang) * 2.0f;
    } else if (held & kButtonDown) {
        delta.x = M_Sine(DEG_0 - ang) * 2.0f;
        delta.y = -M_Cosine(DEG_0 - ang) * 2.0f;
    } else {
        delta.x = 0;
        delta.y = 0;
    }

    pos.x += delta.x;
    pos.y += delta.y;

    playdate->graphics->clear(kColorBlack);
    if (map != NULL) {
        R_DrawSector(playdate->graphics->getFrame(), &map->scts[0], &pos, ang);
    }
    playdate->graphics->markUpdatedRows(0, LCD_ROWS - 1);

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
        sector_stack[0] = &map->scts[0];
    }
    
    return 0;
}
