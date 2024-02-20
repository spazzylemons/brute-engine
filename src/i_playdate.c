#include "a_actor.h"
#include "i_all.h"
#include "r_draw.h"
#include "r_main.h"
#include "u_format.h"
#include "z_memory.h"

// Note on pause menu: The manu button is an undocumented seventh button. Therefore
// we don't have to do anything to implement

PlaydateAPI *playdate;

buttonmask_t I_GetHeldButtons(void) {
    PDButtons buttons;
    playdate->system->getButtonState(&buttons, NULL, NULL);
    return (buttonmask_t) buttons;
}

buttonmask_t I_GetPressedButtons(void) {
    PDButtons buttons;
    playdate->system->getButtonState(NULL, &buttons, NULL);
    return (buttonmask_t) buttons;
}

void I_Log(const char *string) {
    playdate->system->logToConsole("%s", string);
}

uint32_t I_GetMillis(void) {
    return playdate->system->getCurrentTimeMilliseconds();
}

uint8_t *__attribute__((aligned(4))) I_GetFramebuffer(void) {
    return playdate->graphics->getFrame();
}

void I_MarkFramebufferDirty(void) {
    playdate->graphics->markUpdatedRows(0, LCD_ROWS - 1);
}

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

extern map_t *currentMap;

static actor_t *GetActorPointer(void) {
    actor_t *actor = playdate->lua->getArgObject(1, "brute.classes.actor", NULL);
    if (actor == NULL) {
        I_Error("Unregistered actor");
    }
    return actor;
}

static int actor_getPos(lua_State *L) {
    actor_t *actor = GetActorPointer();
    playdate->lua->pushFloat(actor->pos.x);
    playdate->lua->pushFloat(actor->pos.y);
    return 2;
}

static int actor_setPos(lua_State *L) {
    actor_t *actor = GetActorPointer();
    actor->pos.x = playdate->lua->getArgFloat(2);
    actor->pos.y = playdate->lua->getArgFloat(3);
    A_ActorUpdateSector(actor);

    return 0;
}

static int actor_getVel(lua_State *L) {
    actor_t *actor = GetActorPointer();
    playdate->lua->pushFloat(actor->vel.x);
    playdate->lua->pushFloat(actor->vel.y);
    return 2;
}

static int actor_setVel(lua_State *L) {
    actor_t *actor = GetActorPointer();
    actor->vel.x = playdate->lua->getArgFloat(2);
    actor->vel.y = playdate->lua->getArgFloat(3);
    return 0;
}

static int actor_getZPos(lua_State *L) {
    actor_t *actor = GetActorPointer();
    playdate->lua->pushFloat(actor->zpos);
    return 1;
}

static int actor_setZPos(lua_State *L) {
    actor_t *actor = GetActorPointer();
    actor->zpos = playdate->lua->getArgFloat(2);
    return 0;
}

static int actor_getZVel(lua_State *L) {
    actor_t *actor = GetActorPointer();
    playdate->lua->pushFloat(actor->zvel);
    return 1;
}

static int actor_setZVel(lua_State *L) {
    actor_t *actor = GetActorPointer();
    actor->zvel = playdate->lua->getArgFloat(2);
    return 0;
}

static int actor_getAngle(lua_State *L) {
    actor_t *actor = GetActorPointer();
    playdate->lua->pushFloat(actor->angle);
    return 1;
}

static int actor_setAngle(lua_State *L) {
    actor_t *actor = GetActorPointer();
    actor->angle = playdate->lua->getArgFloat(2);
    return 0;
}

static int actor_applyVelocity(lua_State *L) {
    actor_t *actor = GetActorPointer();
    A_ActorApplyVelocity(actor);
    return 0;
}

static int actor_applyGravity(lua_State *L) {
    actor_t *actor = GetActorPointer();
    A_ActorApplyGravity(actor);
    return 0;
}

static int actor_despawn(lua_State *L) {
    actor_t *actor = GetActorPointer();
    A_ActorDespawn(actor);
    return 0;
}

static int actor_free(lua_State *L) {
    actor_t *actor = GetActorPointer();
    Deallocate(actor);
    return 0;
}

static int render_draw(lua_State *L) {
    actor_t *actor = GetActorPointer();
    R_LoadFramebuffer();
    R_RenderViewpoint(actor);
    R_FlushFramebuffer();
    return 0;
}

static int newActor(lua_State *L) {
    vector_t vec;
    vec.x = playdate->lua->getArgFloat(1);
    vec.y = playdate->lua->getArgFloat(2);

    actor_t *actor = A_ActorSpawn(&vec, currentMap);
    playdate->lua->pushObject(actor, "brute.classes.actor", 0);

    return 1;
}

static lua_reg actor_regs[] = {
    { "getPos", actor_getPos },
    { "setPos", actor_setPos },
    { "getVel", actor_getVel },
    { "setVel", actor_setVel },
    { "getZPos", actor_getZPos },
    { "setZPos", actor_setZPos },
    { "getZVel", actor_getZVel },
    { "setZVel", actor_setZVel },
    { "getAngle", actor_getAngle },
    { "setAngle", actor_setAngle },
    { "applyVelocity", actor_applyVelocity },
    { "applyGravity", actor_applyGravity },
    { "despawn", actor_despawn },
    { "free", actor_free },
    { NULL, NULL },
};

static void InitLua(void) {
    // Core functions
    playdate->lua->addFunction(init, "brute.init", NULL);
    playdate->lua->addFunction(quit, "brute.quit", NULL);

    // Actor functions
    playdate->lua->registerClass("brute.classes.actor", actor_regs, NULL, 0, NULL);
    playdate->lua->addFunction(newActor, "brute.newActor", NULL);

    playdate->lua->addFunction(render_draw, "brute.render.draw", NULL);
}

#ifdef _WINDLL
__declspec(dllexport)
#endif
int eventHandler(PlaydateAPI *pd, PDSystemEvent event, uint32_t arg) {
    (void) arg;

    switch (event) {
        case kEventInitLua: {
            playdate = pd;
            InitLua();
            break;
        }

        default: {
            break;
        }
    }
    
    return 0;
}

noreturn void I_Error(const char *fmt, ...) {
    static char errorbuf[256];

    // Print the error message to a buffer.
    va_list list;
    va_start(list, fmt);
    FormatStringV(errorbuf, sizeof(errorbuf), fmt, list);
    va_end(list);

    // Use playdate error handler.
    playdate->system->error("%s", errorbuf);

#ifdef TARGET_SIMULATOR
    exit(1);
#else
    __builtin_unreachable();
#endif
}
