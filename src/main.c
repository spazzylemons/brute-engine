#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>

#include "pd_api.h"

#include "map/load.h"

#include "g_map.h"
#include "r_draw.h"

PlaydateAPI *playdate;

static map_t *map = NULL;

static float ang = 0.0f;
static vertex_t pos = { 0, 0 };
static sector_t *sector = NULL;
static bool draw_3d = true;

#define PI 3.141592653589793f

static int update(void *userdata) {
    (void) userdata;

    PDButtons held, pressed;
    playdate->system->getButtonState(&held, &pressed, NULL);
    if (held & kButtonLeft) {
        ang = fmodf((ang + (2.0f * PI)) - 0.05f, 2.0f * PI);
    }
    if (held & kButtonRight) {
        ang = fmodf(ang + 0.05f, 2.0f * PI);
    }
    vertex_t delta;
    if (held & kButtonUp) {
        delta.x = -sinf(-ang) * 4.0f;
        delta.y = cosf(-ang) * 4.0f;
    } else if (held & kButtonDown) {
        delta.x = sinf(-ang) * 4.0f;
        delta.y = -cosf(-ang) * 4.0f;
    } else {
        delta.x = 0.0f;
        delta.y = 0.0f;
    }
    if (pressed & kButtonA) {
        draw_3d = !draw_3d;
    }

    sector = G_MoveAndSlide(sector, &pos, &delta);

    playdate->graphics->clear(kColorBlack);
    if (sector != NULL) {
        if (draw_3d) {
            R_DrawSector(playdate->graphics->getFrame(), sector, &pos, ang);
        } else {
            R_DrawAutoMap(sector, &pos, ang);
        }
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
            sector = &map->scts[0];
        }
    }
    
    return 0;
}
