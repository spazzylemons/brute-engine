#include "system.h"
#include "video.h"
#include "map/map.h"
#include "render/actor.h"
#include "render/draw.h"
#include "render/local.h"
#include "render/wall.h"
#include "util/file.h"

#include <math.h>
#include <string.h>

// Special value for unblocked column.
#define UNBLOCKED 255
// Special value for fully blocked column.
#define BLOCKED 254

// This may be lowballing it.
#define MAXVISWALLS 128

static uint8_t numviswalls;
static viswall_t viswalls[MAXVISWALLS];

// List of sprite folder names by sprite type.
static const char *const spritenames[NUMSPRITES] = {
    [SPR_TEST] = "test",
};

typedef struct {
    const actor_t *actor;
    int32_t px, py;
} visactor_t;

static visactor_t *actor_array = NULL;
static size_t num_actors = 0;

#define PACKED __attribute__((__packed__))

// File sprite representation.
typedef struct PACKED {
    int16_t offx;         // X offset.
    int16_t offy;         // Y offset.
    uint16_t width;       // Width.
    uint16_t height;      // Height.
    uint32_t postoffs[0]; // Post offsets.
} file_sprite_t;

// A sprite.
typedef struct {
    int16_t offx;
    int16_t offy;
    uint16_t width;
    uint16_t height;
    uint8_t *posts[0];
} sprite_t;

// A sprite frame.
typedef struct {
    // If true, the sprite frame is valid. Only meaningful for initialization and validation.
    bool valid;
    // Bitmask of which sprite frames are unique, so we can free the frames safely.
    uint8_t unique;
    // Bitmask of which sprite frames should be flipped.
    uint8_t flipped;
    // Pointers to sprites for each angle.
    sprite_t *sprites[8];
} spriteframe_t;

// A sprite definition. Defines sprite frames for each animation frame.
typedef struct {
    uint8_t numframes;     // The number of frames.
    spriteframe_t *frames; // An array of frames.
} spritedef_t;

// Array of sprite definitions.
static spritedef_t spritedefs[NUMSPRITES];

static sprite_t *LoadSpriteFromFile(const char *name) {
    // Load sprite file data.
    // TODO we're missing some validation here.
    char *path;
    playdate->system->formatString(&path, "assets/sprites/%s", name);
    size_t size;
    file_sprite_t *fsprite = read_file(path, &size);
    playdate->system->realloc(path, 0);
    if (fsprite->width == 0) {
        // Maybe we could allow this?
        playdate->system->error("Empty sprite");
    }
    uint8_t *fposts = (uint8_t *) fsprite + sizeof(file_sprite_t) + sizeof(uint32_t) * fsprite->width;
    // Calculate total size of posts.
    size_t postsizetotal = size - (sizeof(file_sprite_t) + sizeof(uint32_t) * fsprite->width);
    // Allocate sprite.
    sprite_t *sprite = playdate->system->realloc(NULL, sizeof(sprite_t) + sizeof(uint8_t *) * fsprite->width + postsizetotal);
    sprite->offx = fsprite->offx;
    sprite->offy = fsprite->offy;
    sprite->width = fsprite->width;
    sprite->height = fsprite->height;
    uint8_t *posts = (uint8_t *) sprite + sizeof(sprite_t) + sizeof(uint8_t *) * fsprite->width;
    // Copy posts.
    memcpy(posts, fposts, postsizetotal);
    // Set up pointers.
    for (size_t i = 0; i < fsprite->width; i++) {
        sprite->posts[i] = &posts[fsprite->postoffs[i]];
    }
    // Free file data.
    playdate->system->realloc(fsprite, 0);
    return sprite;
}

static void LoadAngle(sprite_t *sprite, spriteframe_t *frame, char a, bool flipped) {
    int32_t angle = a - '1';
    if (angle < 0 || angle >= 8) {
        playdate->system->error("Angle index out of bounds");
    }
    if (frame->sprites[angle] != NULL) {
        playdate->system->error("Duplicate angle");
    }
    frame->sprites[angle] = sprite;
    if (flipped) {
        frame->flipped |= (1 << angle);
    } else {
        frame->unique |= (1 << angle);
    }
}

static void LoadFrameCallback(const char *name, void *userdata) {
    if (strlen(name) < 6)
        return;

    spriteframe_t *frame = NULL;
    for (int i = 0; i < NUMSPRITES; i++) {
        if (!strncmp(name, spritenames[i], 4)) {
            frame = spritedefs[i].frames;
            break;
        }
    }

    if (frame == NULL)
        return;

    sprite_t *sprite = LoadSpriteFromFile(name);
    // Get angles.
    if (name[5] == '0') {
        // Load all angles with one sprite.
        frame->unique = 1;
        for (uint8_t i = 0; i < 8; i++) {
            frame->sprites[i] = sprite;
        }
    } else {
        LoadAngle(sprite, frame, name[4], false);
        if (name[5]) {
            LoadAngle(sprite, frame, name[5], true);
        }
    }
}

void load_sprites(void) {
    // Iterate through sprite IDs.
    for (spritetype_t i = 0; i < NUMSPRITES; i++) {
        // Get spritedef pointer.
        spritedef_t *spritedef = &spritedefs[i];
        // Allocate frames.
        // TODO determine number of frames!
        spritedef->numframes = 1;
        spritedef->frames = playdate->system->realloc(NULL, sizeof(spriteframe_t) * spritedef->numframes);
        memset(spritedef->frames, 0, sizeof(spriteframe_t) * spritedef->numframes);
        // For each frame...
        playdate->file->listfiles("assets/sprites", LoadFrameCallback, NULL, 0);
    }
}

void free_sprites(void) {
    for (size_t i = 0; i < NUMSPRITES; i++) {
        spritedef_t *spritedef = &spritedefs[i];
        for (size_t j = 0; j < spritedef->numframes; j++) {
            spriteframe_t *frame = &spritedef->frames[j];
            for (uint8_t k = 0; k < 8; k++) {
                if (frame->unique & (1 << k)) {
                    playdate->system->realloc(frame->sprites[k], 0);
                }
            }
            playdate->system->realloc(frame, 0);
        }
    }
}

void R_ClearViswalls(void) {
    numviswalls = 0;
}

viswall_t *R_NewViswall(void) {
    if (numviswalls < MAXVISWALLS) {
        return &viswalls[numviswalls++];
    }
    return NULL;
}

static uint16_t ClipX(int32_t x) {
    if (x < -SCRNDISTI) {
        return 0;
    } else if (x > SCRNDISTI) {
        return SCRNDISTI * 2;
    } else {
        return x + SCRNDISTI;
    }
}

// Minimum Y coordinate for sprite, inclusive.
static uint8_t miny[SCREENWIDTH];
// Maximum Y coordinate for sprite, exclusive.
static uint8_t maxy[SCREENWIDTH];

static uint8_t ClipY(int32_t y, uint16_t x) {
    if (miny[x] == UNBLOCKED) {
        if (y < 0) {
            return 0;
        } else if (y > SCREENHEIGHT) {
            return SCREENHEIGHT;
        } else {
            return y;
        }
    } else {
        if (y < miny[x]) {
            return miny[x];
        } else if (y > maxy[x]) {
            return maxy[x];
        } else {
            return y;
        }
    }
}

static void DrawActor(const visactor_t *visactor) {
    const actor_t *actor = visactor->actor;
    int32_t px = visactor->px;
    int32_t py = visactor->py;
    // Get sprite.
    sprite_t *sprite = spritedefs[SPR_TEST].frames[0].sprites[0];
    fixed_t scale = (SCRNDISTI << FRACBITS) / py;
    // Calculate X bounds.
    fixed_t x1 = scale * (px - sprite->offx);
    fixed_t x2 = x1 + scale * sprite->width;
    int32_t sx1 = (x1 >> FRACBITS) + SCRNDISTI;
    int32_t sx2 = (x2 >> FRACBITS) + SCRNDISTI;
    int32_t sw = sx2 - sx1;
    if (sw == 0) {
        // Don't draw empty sprite.
        return;
    }
    uint16_t minx = ClipX(x1 >> FRACBITS);
    uint16_t maxx = ClipX(x2 >> FRACBITS);

    // Mark all columns as unblocked.
    memset(&miny[minx], UNBLOCKED, maxx - minx);

    // Iterate over sectors from back to front to clip the sprite.
    for (int i = numviswalls - 1; i >= 0; i--) {
        // Check viswall.
        const viswall_t *viswall = &viswalls[i];
        // Check if in bounds of this wall's X coordinates.
        if (minx > viswall->maxx || maxx < viswall->minx) {
            continue;
        }
        // Check if behind this wall.
        if (M_PointInFrontOfWall(viswall->wall, &actor->pos)) {
            continue;
        }
        // Bounds to draw.
        uint16_t wminx = viswall->minx > minx ? viswall->minx : minx;
        uint16_t wmaxx = viswall->maxx < maxx ? viswall->maxx : maxx;
        // If behind, check if portal.
        if (viswall->wall->portal != NULL) {
            // If portal, copy bounds.
            for (uint16_t x = wminx; x < wmaxx; x++) {
                if (miny[x] == UNBLOCKED) {
                    miny[x] = viswall->miny[x - viswall->minx];
                    maxy[x] = viswall->maxy[x - viswall->minx];
                }
            }
        } else {
            // If not portal, mark as clipped.
            for (uint16_t x = wminx; x < wmaxx; x++) {
                if (miny[x] == UNBLOCKED) {
                    miny[x] = BLOCKED;
                }
            }
        }
    }
    // Set scale of texture.
    dc_scale = (py << FRACBITS) / SCRNDISTI;
    // Don't loop texture.
    dc_height = 0x8000;
    // Draw each column.
    fixed_t yoff = fixed_mul(rendereyeheight - float_to_fixed(actor->zpos) - (sprite->offy << FRACBITS), scale);
    for (uint16_t x = minx; x < maxx; x++) {
        if (miny[x] == BLOCKED) {
            // Clipped bound. skip.
            continue;
        }
        // Get the posts of this sprite.
        uint8_t *posts = sprite->posts[(sprite->width * (x - sx1)) / sw];
        uint8_t length;
        // Iterate through the posts.
        uint16_t spot = 0;
        while ((length = *posts++)) {
            spot += *posts++;
            fixed_t yh = yoff + (scale * spot);
            spot += length;
            fixed_t yl = yoff + (scale * spot);
            dc_source = posts;
            dc_x = x;
            dc_yh = ClipY((yh >> FRACBITS) + (SCREENHEIGHT >> 1), x);
            dc_yl = ClipY((yl >> FRACBITS) + (SCREENHEIGHT >> 1), x);
            dc_offset = -fixed_mul(dc_scale, yh);
            R_DrawColumn();
            posts += length;
        }
    }
}

void R_ClearActors(void) {
    if (actor_array != NULL) {
        playdate->system->realloc(actor_array, 0);
        actor_array = NULL;
    }
    num_actors = 0;
}

void R_AddActor(const actor_t *actor) {
    // Bail if set to not render.
    if (actor->flags & ACTOR_NORENDER) {
        return;
    }
    // Translate position locally.
    vector_t pos;
    U_VecCopy(&pos, &actor->pos);
    U_VecSub(&pos, &renderpos);
    R_RotatePoint(&pos);
    int32_t px = floorf(pos.x);
    int32_t py = floorf(pos.y);
    // Don't draw if too close.
    if (py <= 0) {
        return;
    }

    actor_array = playdate->system->realloc(actor_array, sizeof(visactor_t) * (num_actors + 1));

    visactor_t *entry = &actor_array[num_actors++];
    entry->actor = actor;
    entry->px = px;
    entry->py = py;
}

static int SortActors(const void *p, const void *q) {
    const visactor_t *ap = p;
    const visactor_t *aq = q;
    return aq->py - ap->py;
}

void R_DrawActors(void) {
    qsort(actor_array, num_actors, sizeof(visactor_t), SortActors);
    for (size_t i = 0; i < num_actors; i++) {
        DrawActor(&actor_array[i]);
    }
    R_ClearActors();
}
