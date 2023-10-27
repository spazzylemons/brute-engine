#include "i_all.h"
#include "u_error.h"
#include "u_format.h"

#include "pd_api.h"

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

float I_GetCrankAngle(void) {
    return playdate->system->getCrankAngle();
}

bool I_IsCrankDocked(void) {
    return playdate->system->isCrankDocked();
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

static int update(void *userdata) {
    (void) userdata;

    if (haderror) {
        return 0;
    }

    if (CatchError()) {
        haderror = true;
        DisplayError();
    } else {
        B_MainLoop();
    }

    return 1;
}

#ifdef _WINDLL
__declspec(dllexport)
#endif
int eventHandler(PlaydateAPI *pd, PDSystemEvent event, uint32_t arg) {
    (void) arg;

    switch (event) {
        case kEventInit: {
            playdate = pd;
            playdate->system->setUpdateCallback(update, NULL);
            if (CatchError()) {
                haderror = true;
                DisplayError();
            } else {
                B_MainInit();
            }
            break;
        }

        case kEventTerminate: {
            if (!haderror) {
                B_MainQuit();
            }
            break;
        }

        default: {
            break;
        }
    }
    
    return 0;
}
