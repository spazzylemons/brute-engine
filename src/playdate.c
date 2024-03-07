#include "system.h"
#include "video.h"
#include "actor/actor.h"
#include "map/map.h"
#include "render/draw.h"
#include "render/main.h"

PlaydateAPI *playdate;

void B_MainInit(void);
void B_MainQuit(void);

static int init(lua_State *L) {
    B_MainInit();
    return 0;
}

static int quit(lua_State *L) {
    B_MainQuit();
    return 0;
}

static int render_draw(lua_State *L) {
    actor_t *actor = get_actor_pointer();
    R_LoadFramebuffer();
    render_viewpoint(actor);
    R_FlushFramebuffer();
    return 0;
}

#ifdef _WINDLL
__declspec(dllexport)
#endif
int eventHandler(PlaydateAPI *pd, PDSystemEvent event, uint32_t arg) {
    (void) arg;

    switch (event) {
        case kEventInitLua: {
            playdate = pd;

            playdate->lua->addFunction(init, "brute.init", NULL);
            playdate->lua->addFunction(quit, "brute.quit", NULL);
            playdate->lua->addFunction(render_draw, "brute.render.draw", NULL);

            register_actor_class();
            register_map_class();

            break;
        }

        default: {
            break;
        }
    }
    
    return 0;
}
