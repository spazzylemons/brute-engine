#include "i_all.h"
#include "u_error.h"

#include <SDL2/SDL.h>
#include <unistd.h> // temporary

static uint8_t __attribute__((aligned(4))) framebuffer[240 * 52];

static buttonmask_t buttons = 0;

static buttonmask_t TranslateKey(SDL_Keycode code) {
    switch (code) {
        case SDLK_w:     return BTN_U;
        case SDLK_s:     return BTN_D;
        case SDLK_a:     return BTN_L;
        case SDLK_d:     return BTN_R;
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
    return buttons;
}

float I_GetCrankAngle(void) {
    return 0.0f;
}

bool I_IsCrankDocked(void) {
    return true;
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
}

void I_DrawFPS(void) {
}

void B_MainInit(void);
void B_MainLoop(void);
void B_MainQuit(void);

int main(void) {
    memset(framebuffer, 0, sizeof(framebuffer));
    // temporary!
    chdir("Source");

    if (SDL_Init(SDL_INIT_VIDEO)) {
        return EXIT_FAILURE;
    }

    SDL_Window *window;
    SDL_Renderer *renderer;

    SDL_CreateWindowAndRenderer(
        SCREENWIDTH,
        SCREENHEIGHT,
        SDL_WINDOW_RESIZABLE,
        &window,
        &renderer
    );

    SDL_Texture *texture = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_RGBA8888, SDL_TEXTUREACCESS_STREAMING, SCREENWIDTH, SCREENHEIGHT);

    
    if (CatchError()) {
        return EXIT_FAILURE;
    } else {
        B_MainInit();
    }

    uint64_t lastticks = SDL_GetTicks64() * 3;
    for (;;) {
        SDL_Event event;
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_QUIT) {
                B_MainQuit();
                return EXIT_SUCCESS;
            } else if (event.type == SDL_KEYDOWN) {
                buttons |= TranslateKey(event.key.keysym.sym);
            } else if (event.type == SDL_KEYUP) {
                buttons &= ~TranslateKey(event.key.keysym.sym);
            }
        }

        if (CatchError()) {
            return EXIT_FAILURE;
        } else {
            B_MainLoop();
        }


        void *pixels;
        int pitch;
        SDL_LockTexture(texture, NULL, &pixels, &pitch);
        uint32_t *colors = pixels;
        for (int y = 0; y < SCREENHEIGHT; y++) {
            for (int x = 0; x < SCREENWIDTH; x++) {
                int xindex = (y * 52) + (x >> 3);
                int xmask = (1 << (7 - (x & 7)));
                if (framebuffer[xindex] & xmask) {
                    colors[x] = 0xffffffff;
                } else {
                    colors[x] = 0x00000000;
                }
            }
            colors = colors + (pitch / sizeof(uint32_t));
        }
        SDL_UnlockTexture(texture);
        SDL_RenderCopy(renderer, texture, NULL, NULL);
        SDL_RenderPresent(renderer);

        // 30 FPS timing
        while (((SDL_GetTicks64() * 3) - lastticks) < 100) {
            SDL_Delay(1);
        }
        lastticks += 100;
    }
}
