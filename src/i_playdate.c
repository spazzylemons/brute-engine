#include "a_actor.h"
#include "i_all.h"
#include "u_format.h"

#include "pd_api.h"

// Note on pause menu: The manu button is an undocumented seventh button. Therefore
// we don't have to do anything to implement

static PlaydateAPI *playdate;

file_t *I_FileOpen(const char *path, openmode_t mode) {
    FileOptions opts;
    switch (mode) {
        case OPEN_RD:
            opts = kFileRead | kFileReadData;
            break;
        case OPEN_WR:
            opts = kFileWrite;
            break;
        default:
            return NULL;
    }

    return (file_t *) playdate->file->open(path, opts);
}

void I_FileClose(file_t *file) {
    playdate->file->close((SDFile *) file);
}

void I_FileSeek(file_t *file, int32_t amt, int whence) {
    if (playdate->file->seek((SDFile *) file, amt, whence) < 0) {
        I_Error("I_FileSeek failed");
    }
}

uint32_t I_FileTell(file_t *file) {
    int result = playdate->file->tell((SDFile *) file);
    if (result < 0) {
        I_Error("I_FileTell failed");
    }
    return result;
}

uint32_t I_FileRead(file_t *file, void *buffer, uint32_t size) {
    return playdate->file->read((SDFile *) file, buffer, size);
}

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

// Deadzone in degrees.
#define DEADZONE 10.0f

float I_GetAnalogStrength(void) {
    float crankangle = playdate->system->getCrankAngle();
    if (crankangle <= 180.0f) {
        // Deadzone of ten degrees either way.
        float anglechange;
        if (crankangle < 90.0f - DEADZONE) {
            anglechange = (90.0f - DEADZONE) - crankangle;
        } else if (crankangle > 90.0f + DEADZONE) {
            anglechange = (90.0f + DEADZONE) - crankangle;
        } else {
            anglechange = 0.0f;
        }
        return anglechange;
    } else {
        return 0.0f;
    }
}

bool I_HasAnalogInput(void) {
    return !playdate->system->isCrankDocked();
}

void *I_Malloc(size_t size) {
    return playdate->system->realloc(NULL, size);
}

void I_Free(void *ptr) {
    playdate->system->realloc(ptr, 0);
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

void I_DrawFPS(void) {
    playdate->system->drawFPS(0, 0);
}

void B_MainInit(void);
void B_MainLoop(void);
void B_MainQuit(void);

static int init(lua_State *L) {
    B_MainInit();
    return 0;
}

static int update(lua_State *L) {
    B_MainLoop();
    return 0;
}

static int quit(lua_State *L) {
    B_MainQuit();
    return 0;
}

extern map_t *currentMap;

static actor_t *GetActorPointer(void) {
    actor_t *actor = playdate->lua->getArgObject(1, "bruteclass.actor", NULL);
    if (actor == NULL) {
        I_Error("Unregistered actor");
    }
    return actor;
}

static int actor_register(lua_State *L) {
    vector_t vec;
    vec.x = playdate->lua->getArgFloat(1);
    vec.y = playdate->lua->getArgFloat(2);

    actor_t *actor = A_ActorSpawn(&vec, currentMap);
    playdate->lua->pushObject(actor, "bruteclass.actor", 0);

    return 1;
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

static void InitLua(void) {
    // Core functions
    playdate->lua->addFunction(init, "brute.init", NULL);
    playdate->lua->addFunction(update, "brute.update", NULL);
    playdate->lua->addFunction(quit, "brute.quit", NULL);

    // Actor functions
    playdate->lua->registerClass("bruteclass.actor", NULL, NULL, 0, NULL);
    playdate->lua->addFunction(actor_register, "brute.actor.register", NULL);
    playdate->lua->addFunction(actor_getPos, "brute.actor.getPos", NULL);
    playdate->lua->addFunction(actor_setPos, "brute.actor.setPos", NULL);
    playdate->lua->addFunction(actor_getVel, "brute.actor.getVel", NULL);
    playdate->lua->addFunction(actor_setVel, "brute.actor.setVel", NULL);
    playdate->lua->addFunction(actor_getZPos, "brute.actor.getZPos", NULL);
    playdate->lua->addFunction(actor_setZPos, "brute.actor.setZPos", NULL);
    playdate->lua->addFunction(actor_getZVel, "brute.actor.getZVel", NULL);
    playdate->lua->addFunction(actor_setZVel, "brute.actor.setZVel", NULL);
    playdate->lua->addFunction(actor_getAngle, "brute.actor.getAngle", NULL);
    playdate->lua->addFunction(actor_setAngle, "brute.actor.setAngle", NULL);
    playdate->lua->addFunction(actor_applyVelocity, "brute.actor.applyVelocity", NULL);
    playdate->lua->addFunction(actor_applyGravity, "brute.actor.applyGravity", NULL);
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
