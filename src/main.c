#include <stdio.h>
#include <stdlib.h>

#include "pd_api.h"

static PlaydateAPI *playdate;

static int update(void *userdata) {
    (void) userdata;

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
    }
    
    return 0;
}
