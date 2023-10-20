/**
 * Abstracts Playdate init and update loop.
 */

void B_MainInit(void);

void B_MainLoop(void);

#include "pd_api.h"
#include "u_error.h"

#include <stdbool.h>

// Playdate API access.
PlaydateAPI *playdate;
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

    if (event == kEventInit) {
        playdate = pd;
        playdate->system->setUpdateCallback(update, NULL);
        if (CatchError()) {
            haderror = true;
            DisplayError();
        } else {
            B_MainInit();
        }
    }
    
    return 0;
}
