#include "b_core.h"
#include "r_draw.h"
#include "r_flat.h"
#include "r_local.h"
#include "r_wall.h"
#include "u_vec.h"

static const wall_t *renderwall; // The current wall.
static float wallsine;           // Sine multiply for wall rotation.
static float wallcosine;         // Cosine multiply for wall rotation.

static fixed_t heightceiling; // Height to draw ceiling at.
static fixed_t heightfloor;   // Height to draw floor at.

static int32_t distleft;  // Distance of left side of wall.
static int32_t distright; // Distance of right side of wall.

static int32_t uvleft;  // Left UV X coordinate.
static int32_t uvright; // Right UV X coordinate.

static fixed_t rendereyeheight; // Eye height to render at.

// Enum for how to clip sector bounds.
typedef enum {
    CLIP_NONE,  // Perform no clipping.
    CLIP_CEIL,  // Clip the ceiling Y bounds.
    CLIP_FLOOR, // Clip the floor Y bounds.
} cliptype_t;

// Buffer for storing Y pixel coordinate of portal top.
static uint8_t renderminy[LCD_COLUMNS];
// Buffer for storing Y pixel coordinate of portal bottom.
static uint8_t rendermaxy[LCD_COLUMNS];
// Buffer for storing previous Y pixel top, for flat drawing.
static uint8_t prevminy[LCD_COLUMNS];
// Buffer for storing previous Y pixel bottom, for flat drawing.
static uint8_t prevmaxy[LCD_COLUMNS];

static void TryClip(cliptype_t cliptype, int32_t val, int32_t x) {
    if (val < 0) {
        val = 0;
    } else if (val > LCD_ROWS) {
        val = LCD_ROWS;
    }

    switch (cliptype) {
        case CLIP_NONE:
            break;
        case CLIP_CEIL:
            if (val > rendermaxy[x]) {
                val = rendermaxy[x];
            }
            if (val > renderminy[x]) {
                renderminy[x] = val;
            }
            break;
        case CLIP_FLOOR:
            if (val < renderminy[x]) {
                val = renderminy[x];
            }
            if (val < rendermaxy[x]) {
                rendermaxy[x] = val;
            }
            break;
    }
}

static void RotatePoint(vector_t *v) {
    float x = v->x * wallcosine - v->y * wallsine;
    v->y = v->x * wallsine + v->y * wallcosine;
    v->x = x;
}

static void DrawFlats(void) {
    R_DrawFlat(rendersector->ceilflat, prevminy, renderminy, heightceiling);
    R_DrawFlat(rendersector->floorflat, rendermaxy, prevmaxy, heightfloor);
}

static uint8_t ClipYPoint(int32_t y, int32_t x) {
    if (y < renderminy[x]) {
        return renderminy[x];
    } else if (y > rendermaxy[x]) {
        return rendermaxy[x];
    }
    return y;
}

static void SetColumnOffset(int32_t height, const patch_t *patch) {
    if (patch != NULL) {
        dc_offset = (renderwall->yoffset << FRACBITS) -
            (height & (((1 << FRACBITS) - 1) | ((patch->height - 1) << FRACBITS)));
    }
}

static void DrawWallColumns(
    const patch_t *patch,
    cliptype_t ceilclip,
    cliptype_t floorclip
) {
    // Location of wall endpoints.
    fixed_t hfrac = (SCRNDISTI * heightceiling) / distleft;
    fixed_t lfrac = (SCRNDISTI * heightfloor) / distleft;
    // X distance to draw.
    uint16_t dx = renderxmax - renderxmin;
    // Factor to scale wall endpoint step values.
    int32_t stepnum = SCRNDISTI * (distleft - distright);
    int32_t stepden = distleft * distright * dx;
    fixed_t stepfac = (stepnum << FRACBITS) / stepden;
    // Amount to move wall endpoints after each column.
    fixed_t hstep = R_FixedMul(heightceiling, stepfac);
    fixed_t lstep = R_FixedMul(heightfloor, stepfac);
    // Texture scaling values.
    fixed_t scale = (SCRNDISTI << INTBITS) / distleft;
    fixed_t scalestep = stepfac << (INTBITS - FRACBITS);
    // Texture drawing values.
    int32_t uend = distright * dx;
    int32_t du = distleft * (uvright - uvleft);
    int32_t dz = distright - distleft;
    // Preset height of texture.
    if (patch != NULL) {
        dc_height = patch->height;
    }
    // Wall drawing loop.
    for (uint16_t x = renderxmin; x < renderxmax; x++) {
        // Find integer endpoints on screen.
        int32_t yh = (hfrac >> FRACBITS) + (LCD_ROWS >> 1);
        int32_t yl = (lfrac >> FRACBITS) + (LCD_ROWS >> 1);
        // Draw patch if present.
        if (patch != NULL) {
            // Calculate which column to render.
            uint16_t x1 = x - renderxmin;
            int32_t num = du * x1;
            int32_t den = uend - x1 * dz;
            uint16_t whichx = ((uvleft + (num / den)) >> 4) & (patch->width - 1);
            // Set parameters.
            dc_source = &patch->data[whichx * patch->height];
            dc_scale = 0xffffffffu / (uint32_t) scale;
            dc_x = x;
            // Check bounds.
            dc_yh = ClipYPoint(yh, x);
            dc_yl = ClipYPoint(yl, x);
            // Draw the column.
            R_DrawColumn();
        }
        // Update bounds.
        prevminy[x] = renderminy[x];
        prevmaxy[x] = rendermaxy[x];
        TryClip(ceilclip, yh, x);
        TryClip(floorclip, yl, x);
        // Step accumulators.
        hfrac += hstep;
        lfrac += lstep;
        scale += scalestep;
    }
}

// Clip floating-point X bound to integer bound within range of screen (right side exclusive)
static uint16_t ClipToScreen(float b) {
    if (b < -SCRNDIST) {
        return 0;
    } else if (b > SCRNDIST) {
        return SCRNDISTI * 2;
    } else {
        return (int16_t) floorf(b) + SCRNDISTI;
    }
}

// Clip floating-point Y distance to integer distance, avoiding 0 divide.
static int32_t ClipDist(float d) {
    int32_t result = floorf(d);
    if (result == 0) {
        return 1;
    }
    return result;
}

typedef struct {
    float bound;
    float uv;
    vector_t vec;
} clip_t;

static float ClipEndpoint(
    const clip_t *c1,
    const clip_t *c2,
    float val1,
    float val2,
    vector_t *destvec,
    float *uv
) {
    if (val1 < 0.0f) {
        if (val2 < 0.0f) {
            U_VecCopy(destvec, &c2->vec);
            *uv = c2->uv;
            return c2->bound;
        } else {
            return (SCRNDIST * destvec->x) / destvec->y;
        }
    } else {
        U_VecCopy(destvec, &c1->vec);
        *uv = c1->uv;
        return c1->bound;
    }
}

static bool ClipWall(float *left, float *right) {
    // Find the corners of this wall.
    vector_t a, b;
    U_VecCopy(&a, renderwall->v1);
    U_VecCopy(&b, renderwall->v2);
    // Offset by render position.
    U_VecSub(&a, &renderpos);
    U_VecSub(&b, &renderpos);
    // Rotate by render angle.
    RotatePoint(&a);
    RotatePoint(&b);
    // Check clipping on left side of frustrum.
    clip_t cl;
    cl.bound = *left;
    float al = (cl.bound * a.y) - (SCRNDIST * a.x);
    float bl = (SCRNDIST * b.x) - (cl.bound * b.y);
    if (al > 0.0f && bl < 0.0f) {
        // Line is fully clipped on left side.
        return false;
    }
    // Check clipping on right side of frustrum.
    clip_t cr;
    cr.bound = *right;
    float ar = (cr.bound * a.y) - (SCRNDIST * a.x);
    float br = (SCRNDIST * b.x) - (cr.bound * b.y);
    if (ar < 0.0f && br > 0.0f) {
        // Line is fully clipped on right side.
        return false;
    }
    // We might divide by zero here, but it's fine because 1. it's floating point
    // and 2. in such a case, we'd never actually use these values (line segment
    // parallel to clipping line)
    float ta = al / (al + bl);
    float tb = ar / (ar + br);
    // Find both points where the line intersects the frustrum bounds.
    vector_t d;
    U_VecCopy(&d, &b);
    U_VecSub(&d, &a);
    U_VecCopy(&cl.vec, &a);
    U_VecCopy(&cr.vec, &a);
    U_VecScaledAdd(&cl.vec, &d, ta);
    U_VecScaledAdd(&cr.vec, &d, tb);
    // Find UV coordinates at these points.
    float ua = renderwall->xoffset;
    float ub = ua + renderwall->length;
    cl.uv = ua + ta * renderwall->length;
    cr.uv = ua + tb * renderwall->length;
    // Clip left endpoint.
    float ncl = ClipEndpoint(&cl, &cr, al, ar, &a, &ua);
    if (a.y <= 0.0f) {
        // Left endpoint ended up behind camera. Stop.
        return false;
    }
    // Clip right endpoint.
    float ncr = ClipEndpoint(&cr, &cl, br, bl, &b, &ub);
    if (b.y <= 0.0f) {
        // Right endpoint ended up behind camera. Stop.
        return false;
    }
    // Backface culling.
    if ((a.x * b.y) > (b.x * a.y)) {
        return false;
    }
    // Finalize the integer endpoints of the rendered wall.
    renderxmin = ClipToScreen(ncl);
    renderxmax = ClipToScreen(ncr);
    // Stop if wall is empty.
    if (renderxmin == renderxmax) {
        return false;
    }
    // Get the integer distance of the endpoints of the rendered wall.
    distleft = ClipDist(a.y);
    distright = ClipDist(b.y);
    // Get the integer UV coordinates of the texture.
    uvleft = floorf(ua * 16.0f);
    uvright = floorf(ub * 16.0f);
    // Update bounds if this is a portal.
    if (renderwall->portal != NULL) {
        *left = ncl;
        *right = ncr;
    }
    // Wall can be rendered!
    return true;
}

void R_InitWallGlobals(float angle, float eyeheight) {
    // Precalculate sine and cosine.
    wallsine = sinf(-angle);
    wallcosine = cosf(-angle);
    // Initialize min and max buffers.
    memset(renderminy, 0, sizeof(renderminy));
    memset(rendermaxy, LCD_ROWS, sizeof(rendermaxy));
    memset(prevminy, 0, sizeof(prevminy));
    memset(prevmaxy, LCD_ROWS, sizeof(prevmaxy));
    // Remember eye height.
    rendereyeheight = R_FloatToFixed(eyeheight);
}

bool R_DrawWall(const wall_t *wall, float *left, float *right) {
    renderwall = wall;

    if (!ClipWall(left, right)) {
        return false;
    }

    heightceiling = rendereyeheight - (rendersector->ceiling << FRACBITS);
    heightfloor = rendereyeheight - (rendersector->floor << FRACBITS);
    SetColumnOffset(heightceiling, renderwall->midpatch);

    DrawWallColumns(renderwall->midpatch, CLIP_CEIL, CLIP_FLOOR);
    DrawFlats();

    if (renderwall->portal != NULL) {
        // If the height of the ceiling goes down, render top wall.
        if (renderwall->portal->ceiling < rendersector->ceiling) {
            heightfloor = rendereyeheight - (renderwall->portal->ceiling << FRACBITS);
            SetColumnOffset(heightfloor, renderwall->toppatch);
            DrawWallColumns(renderwall->toppatch, CLIP_NONE, CLIP_CEIL);
        }

        // If the height of the floor goes up, render bottom wall.
        if (renderwall->portal->floor > rendersector->floor) {
            heightceiling = rendereyeheight - (renderwall->portal->floor << FRACBITS);
            heightfloor = rendereyeheight - (rendersector->floor << FRACBITS);
            SetColumnOffset(heightceiling, renderwall->botpatch);
            DrawWallColumns(renderwall->botpatch, CLIP_FLOOR, CLIP_NONE);
        }

        return true;
    } else {
        return false;
    }
}
