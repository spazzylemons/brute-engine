#include "i_all.h"
#include "u_error.h"

#include <SDL2/SDL.h>
#ifdef __EMSCRIPTEN__
#include <emscripten.h>
#include <emscripten/html5.h>
#else
#include <unistd.h> // temporary
#endif
#include <stdlib.h>

static uint8_t __attribute__((aligned(4))) framebuffer[240 * 52];

static buttonmask_t buttons = 0;

static SDL_Window *window;
static SDL_Renderer *renderer;
static SDL_Texture *texture;
static float mousemotion = 0.0f;

static buttonmask_t TranslateKey(SDL_Keycode code) {
    switch (code) {
        // WASD
        case SDLK_w:     return BTN_U;
        case SDLK_s:     return BTN_D;
        case SDLK_a:     return BTN_L;
        case SDLK_d:     return BTN_R;
        // Arrow keys
        case SDLK_UP:    return BTN_U;
        case SDLK_DOWN:  return BTN_D;
        case SDLK_LEFT:  return BTN_L;
        case SDLK_RIGHT: return BTN_R;
        // Any other key not yet mapped
        default:         return 0;
    }
}

file_t *I_FileOpen(const char *path, openmode_t mode) {
    const char *modestr;
    switch (mode) {
        case OPEN_RD:
            modestr = "rb";
            break;
        case OPEN_WR:
            modestr = "wb";
            break;
        default:
            return NULL;
    }

    return (file_t *) fopen(path, modestr);
}

void I_FileClose(file_t *file) {
    fclose((FILE *) file);
}

void I_FileSeek(file_t *file, int32_t amt, int whence) {
    if (fseek((FILE *) file, amt, whence) < 0) {
        Error("I_FileSeek failed");
    }
}

uint32_t I_FileTell(file_t *file) {
    // TODO overflow protection
    return ftell((FILE *) file);
}

uint32_t I_FileRead(file_t *file, void *buffer, uint32_t size) {
    return fread(buffer, 1, size, (FILE *) file);
}

buttonmask_t I_GetHeldButtons(void) {
    buttonmask_t result = buttons;
    // Mask out invalid inputs.
    if ((result & (BTN_L | BTN_R)) == (BTN_L | BTN_R)) {
        result &= ~(BTN_L | BTN_R);
    }
    if ((result & (BTN_U | BTN_D)) == (BTN_U | BTN_D)) {
        result &= ~(BTN_U | BTN_D);
    }
    return result;
}

float I_GetAnalogStrength(void) {
    return mousemotion;
}

bool I_HasAnalogInput(void) {
    return SDL_GetRelativeMouseMode();
}

void *I_Malloc(size_t size) {
    return malloc(size);
}

void I_Free(void *ptr) {
    free(ptr);
}

void I_Log(const char *string) {
    printf("%s\n", string);
}

uint32_t I_GetMillis(void) {
    return SDL_GetTicks();
}

uint8_t *__attribute__((aligned(4))) I_GetFramebuffer(void) {
    return framebuffer;
}

void I_MarkFramebufferDirty(void) {
    void *pixels;
    int pitch;
    SDL_LockTexture(texture, NULL, &pixels, &pitch);
    uint16_t *colors = pixels;
    for (int y = 0; y < SCREENHEIGHT; y++) {
        for (int x = 0; x < SCREENWIDTH; x++) {
            int xindex = (y * 52) + (x >> 3);
            int xmask = (1 << (7 - (x & 7)));
            if (framebuffer[xindex] & xmask) {
                colors[x] = 0xffff;
            } else {
                colors[x] = 0x0000;
            }
        }
        colors = colors + (pitch / sizeof(uint16_t));
    }
    SDL_UnlockTexture(texture);
    SDL_RenderCopy(renderer, texture, NULL, NULL);
    SDL_RenderPresent(renderer);
}

void I_DrawFPS(void) {
}

void B_MainInit(void);
void B_MainLoop(void);
void B_MainQuit(void);

static bool haserror = false;

static bool loop(void) {
    // Reset mouse motion from previous frame.
    mousemotion = 0.0f;

    // Run events.
    SDL_Event event;
    while (SDL_PollEvent(&event)) {
        switch (event.type) {
            case SDL_QUIT: {
                if (!haserror) {
                    B_MainQuit();
                }
                return false;
            }

            case SDL_KEYDOWN: {
                // If escape pressed, un-capture the mouse.
                if (event.key.keysym.sym == SDLK_ESCAPE) {
                    // Stop capturing the mouse.
                    SDL_SetRelativeMouseMode(false);
                }

                buttons |= TranslateKey(event.key.keysym.sym);
                break;
            }

            case SDL_KEYUP: {
                buttons &= ~TranslateKey(event.key.keysym.sym);
                break;
            }

            case SDL_MOUSEBUTTONDOWN: {
                if (event.button.button == SDL_BUTTON_LEFT) {
                    // Capture the mouse.
                    SDL_SetRelativeMouseMode(true);
                }
                break;
            }

            case SDL_MOUSEMOTION: {
                // Have we captured the mouse?
                if (SDL_GetRelativeMouseMode()) {
                    // If so, read this motion.
                    // We scale it by a factor that makes it feel less sluggish,
                    // and that flips the sign because adding to an angle moves
                    // counter-clockwise.
                    mousemotion = event.motion.xrel * -1.5f;
                }
                break;
            }
        }
    }

    if (!haserror) {
        if (CatchError()) {
            haserror = true;
        } else {
            B_MainLoop();
            return true;
        }
    }

    return true;
}

#ifdef __EMSCRIPTEN__
static void emloop(void) {
    loop();
}
#endif

int main(void) {
    memset(framebuffer, 0, sizeof(framebuffer));

#ifndef __EMSCRIPTEN__
    // temporary hack! for a real PC version we'd want to load from the *same*
    // directory as the executable.
    chdir("Source");
#endif

    if (SDL_Init(SDL_INIT_VIDEO)) {
        return EXIT_FAILURE;
    }

    SDL_CreateWindowAndRenderer(
        SCREENWIDTH * 2,
        SCREENHEIGHT * 2,
        SDL_WINDOW_RESIZABLE,
        &window,
        &renderer
    );

    texture = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_RGB565, SDL_TEXTUREACCESS_STREAMING, SCREENWIDTH, SCREENHEIGHT);

    
    if (CatchError()) {
        haserror = true;
    } else {
        B_MainInit();
    }

#ifdef __EMSCRIPTEN__
    emscripten_set_main_loop(emloop, 30, 0);
    return 0;
#else
    uint64_t lastticks = SDL_GetTicks64() * 3;
    while (loop()) {
        // 30 FPS timing
        while (((SDL_GetTicks64() * 3) - lastticks) < 100) {
            SDL_Delay(1);
        }
        lastticks += 100;
    }
#endif
}
