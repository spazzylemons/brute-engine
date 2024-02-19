#include "i_all.h"
#include "u_error.h"
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
        Error("I_FileSeek failed");
    }
}

uint32_t I_FileTell(file_t *file) {
    int result = playdate->file->tell((SDFile *) file);
    if (result < 0) {
        Error("I_FileTell failed");
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

// If true, an error occurred.
static bool haderror = false;

static int init(lua_State *L) {
    (void) L;

    if (CatchError()) {
        haderror = true;
    } else {
        B_MainInit();
    }

    return 0;
}

static int update(lua_State *L) {
    (void) L;

    if (haderror) {
        return 0;
    }

    if (CatchError()) {
        haderror = true;
    } else {
        B_MainLoop();
    }

    return 0;
}

static int quit(lua_State *L) {
    (void) L;

    if (!haderror) {
        B_MainQuit();
    }

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
            playdate->lua->addFunction(update, "brute.update", NULL);
            playdate->lua->addFunction(quit, "brute.quit", NULL);
            break;
        }

        default: {
            break;
        }
    }
    
    return 0;
}
