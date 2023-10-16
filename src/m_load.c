#include "b_core.h"
#include "m_load.h"
#include "u_error.h"
#include "u_file.h"
#include "u_format.h"
#include "u_vec.h"

// Just in case, we'll pack the structs we read from the files.
#define PACKED __attribute__((__packed__))

// Format of vertex used in file.
typedef struct PACKED {
    // The X coordinate of the vertex.
    int16_t x;
    // The Y coordinate of the vertex.
    int16_t y;
} file_vertex_t;

// Format of sector used in file.
typedef struct PACKED {
    // The number of walls.
    uint16_t num_walls;
    // The first wall index.
    uint16_t first_wall;
} file_sector_t;

// Format of a wall used in file.
typedef struct PACKED {
    // The vertex of the first point of this wall. The next point of this wall is implicitly the
    // first point of the next wall in the sector.
    uint16_t vertex;
    // Sector that this wall is a portal to. If the portal sector ID is the same as the sector ID
    // that this wall is in, then it is not a portal.
    uint16_t portal;
    // The texture X offset.
    uint8_t xoffset;
} file_wall_t;

static void *ReadMapFile(const char *name, const char *file, size_t *szp, size_t mbsz) {
    // Format map path.
    char pathbuf[64];
    FormatString(pathbuf, sizeof(pathbuf), "/maps/%s/%s", name, file);
    // Read file.
    void *result = U_FileRead(pathbuf, szp);
    // Divide size by member count.
    *szp /= mbsz;
    // Return file contents.
    return result;
}

static void LoadVertices(const char *name, map_t *map) {
    file_vertex_t *fvtxs = ReadMapFile(name, "vertices", &map->numvtxs, sizeof(file_vertex_t));
    // Allocate vertices.
    map->vtxs = Allocate(sizeof(vector_t) * map->numvtxs);
    // Convert vertices.
    for (size_t i = 0; i < map->numvtxs; i++) {
        map->vtxs[i].x = fvtxs[i].x;
        map->vtxs[i].y = fvtxs[i].y;
    }
    // Free file data.
    Deallocate(fvtxs);
}

static void LoadWalls(const char *name, map_t *map) {
    file_wall_t *fwalls = ReadMapFile(name, "walls", &map->numwalls, sizeof(file_wall_t));
    // Allocate walls.
    map->walls = Allocate(sizeof(wall_t) * map->numwalls);
    // Convert walls. Not all data is validated until sectors are converted.
    for (size_t i = 0; i < map->numwalls; i++) {
        file_wall_t *fwall = &fwalls[i];
        wall_t *wall = &map->walls[i];
        // Check bounds of wall vertex.
        if (fwall->vertex >= map->numvtxs) {
            Error("M_Load: Vertices of wall %d are out of bounds", i);
        }
        // Store vertex 1. Vertex 2 cannot be set until sectors are converted.
        wall->v1 = &map->vtxs[fwall->vertex];
        // Store the portal index directly into the portal pointer. The sector
        // conversion routine will finish the conversion.
        wall->portal = (void *) (uintptr_t) fwall->portal;
        // Store the wall patch.
        wall->patch = map->patches;
        // Store the wall's offsets.
        wall->xoffset = fwall->xoffset;
    }
    // Free file data.
    Deallocate(fwalls);
}

static void LoadSectors(const char *name, map_t *map) {
    file_sector_t *fscts = ReadMapFile(name, "sectors", &map->numscts, sizeof(file_sector_t));
    // This error will be obsolete once actors are supported.
    if (map->numscts == 0) {
        Error("Map has no sectors.");
    }
    // Allocate sectors.
    map->scts = Allocate(sizeof(sector_t) * map->numscts);
    // Convert sectors.
    for (size_t i = 0; i < map->numscts; i++) {
        file_sector_t *fsector = &fscts[i];
        sector_t *sector = &map->scts[i];
        // Check that the sector is a polygon.
        if (fsector->num_walls < 3) {
            Error("M_Load: Sector %d is not a polygon", i);
        }
        // Check that the wall slice is in bounds.
        size_t wstart = fsector->first_wall;
        size_t wend = wstart + fsector->num_walls;
        if (wend > map->numwalls) {
            Error("M_Load: Walls of sector %d are out of bounds", i);
        }
        // Finish converting the walls, and find the bounding box.
        sector->walls = &map->walls[fsector->first_wall];
        sector->num_walls = fsector->num_walls;
        sector->bounds.min.x = INFINITY;
        sector->bounds.min.y = INFINITY;
        sector->bounds.max.x = -INFINITY;
        sector->bounds.max.y = -INFINITY;
        for (size_t j = 0; j < sector->num_walls; j++) {
            wall_t *wall = &sector->walls[j];
            wall_t *next = &sector->walls[(j + 1) % sector->num_walls];
            wall->v2 = next->v1;
            // Wall must have a nonzero length.
            if (U_VecDistSq(wall->v1, wall->v2) == 0.0f) {
                Error("M_Load: Wall %d of sector %d has zero length", j, i);
            }
            // Precalculate delta.
            U_VecCopy(&wall->delta, wall->v2);
            U_VecSub(&wall->delta, wall->v1);
            // Precalculate normal.
            wall->normal.x = wall->delta.y;
            wall->normal.y = -wall->delta.x;
            U_VecNormalize(&wall->normal);
            // Precalculate length.
            wall->length = sqrtf(U_VecLenSq(&wall->delta));
            // Check for bounding box.
            AABBExpandToFit(&sector->bounds, wall->v1);
            // Wall portal index must be within bounds.
            size_t portalindex = (uintptr_t) wall->portal;
            if (portalindex != i) {
                if (portalindex >= map->numscts) {
                    Error("M_Load: Portal index of wall %d of sector %d is out of bounds", j, i);
                }
                wall->portal = &map->scts[portalindex];
            } else {
                wall->portal = NULL;
            }
        }
        // Initialize iterator lists.
        sector->next_seen = NULL;
        sector->next_queue = NULL;
    }
    // Free file data.
    Deallocate(fscts);
}

// Load a patch from a bitmap.
static patch_t *LoadPatch(const char *path) {
    // Load the bitmap.
    const char *err = NULL;
    LCDBitmap *bitmap = playdate->graphics->loadBitmap(path, &err);
    if (bitmap == NULL) {
        Error("M_Load: Failed to load patch '%s': %s", path, err);
    }
    // Get the texture dimensions, checking bounds.
    int width, height, rowbytes;
    uint8_t *mask, *data;
    playdate->graphics->getBitmapData(bitmap, &width, &height, &rowbytes, &mask, &data);
    if (width > UINT16_MAX || height > UINT16_MAX) {
        Error("M_Load: Patch '%s' too big", path);
    }
    // Enforce a power of two - this makes texture mapping faster.
    if ((width & (width - 1)) || (height & (height - 1))) {
        Error("M_Load: Patch '%s' must have power-of-two dimensions", path);
    }
    // Allocate patch.
    uint16_t stride = (height + 7) >> 3;
    size_t datasize = stride * width;
    patch_t *patch = Allocate(sizeof(patch_t) + datasize);
    patch->next = NULL;
    patch->width = width;
    patch->height = height;
    patch->stride = stride;
    // Convert to columns.
    memset(&patch->data[0], 0, datasize);
    for (size_t x = 0; x < width; x++) {
        uint8_t xmask = 1 << (7 - (x & 7));
        size_t xindex = x >> 3;
        for (size_t y = 0; y < height; y++) {
            if (data[(y * rowbytes) + xindex] & xmask) {
                uint8_t mask = 1 << (7 - (y & 7));
                size_t index = y >> 3;
                patch->data[(x * stride) + index] |= mask;
            }
        }
    }
    // Free the bitmap.
    playdate->graphics->freeBitmap(bitmap);
    // Return the new patch.
    return patch;
}

map_t *M_Load(const char *name) {
    // Allocate map.
    map_t *map = Allocate(sizeof(map_t));
    // Load the texture.
    map->patches = LoadPatch("textures/wall");
    // Load each part of the map.
    LoadVertices(name, map);
    LoadWalls(name, map);
    LoadSectors(name, map);
    return map;
}

void M_Free(map_t *map) {
    Deallocate(map->vtxs);
    Deallocate(map->walls);
    Deallocate(map->scts);
    Deallocate(map->patches);
    Deallocate(map);
}

static bool PointInFrontOfWall(const wall_t *wall, const vector_t *point) {
    return (wall->v2->y - wall->v1->y) * (point->x - wall->v1->x) >
        (wall->v2->x - wall->v1->x) * (point->y - wall->v1->y);
}

bool M_SectorContainsPoint(const sector_t *sector, const vector_t *point) {
    for (size_t i = 0; i < sector->num_walls; i++) {
        const wall_t *wall = &sector->walls[i];
        // Test if point lies in front of line.
        if (!PointInFrontOfWall(wall, point)) {
            return false;
        }
    }
    return true;
}
