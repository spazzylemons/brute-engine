#include "i_system.h"
#include "m_load.h"
#include "u_file.h"
#include "u_format.h"
#include "u_vec.h"
#include "w_pack.h"
#include "z_memory.h"

#include <math.h>
#include <string.h>

// Just in case, we'll pack the structs we read from the files.
#define PACKED __attribute__((__packed__))

// Format of patch used in file.
typedef struct PACKED {
    // Packed width and height.
    uint8_t dimensions;
    // The patch data, stored in columns.
    uint8_t data[0];
} file_patch_t;

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
    // The floor height.
    int16_t floor;
    // The ceiling height.
    int16_t ceiling;
    // The flat ID for the floor.
    uint8_t floorflat;
    // The flat ID for the ceiling.
    uint8_t ceilflat;
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
    // The texture Y offset.
    uint8_t yoffset;
    // The top texture patch ID.
    uint8_t textop;
    // The middle texture patch ID.
    uint8_t texmid;
    // The bottom texture patch ID.
    uint8_t texbot;
} file_wall_t;

static uint32_t branchflats;   // Branch containing flats.
static uint32_t branchmaps;    // Branch containing maps.
static uint32_t branchpatches; // Branch containing patches.

void M_Init(void) {
    branchflats = W_GetNumByName(ROOTID, "flats");
    branchmaps = W_GetNumByName(ROOTID, "maps");
    branchpatches = W_GetNumByName(ROOTID, "patches");
}

static void *ReadMapFile(uint32_t branch, const char *file, size_t *szp, size_t mbsz) {
    // Read file.
    void *result = W_ReadLump(W_GetNumByName(branch, file), szp);
    // Divide size by member count.
    *szp /= mbsz;
    // Return file contents.
    return result;
}

// Load a patch from a bitmap.
static void LoadPatch(patch_t *patch, const char *name) {
    size_t size;
    file_patch_t *fpatch = W_ReadLump(
        W_GetNumByName(branchpatches, name),
        &size
    );
    // Verify the allocation at least has the dimensions.
    if (size < 1) {
        I_Error("M_Load: Patch missing dimensions");
    }
    // Get the dimensions.
    uint16_t width = 1 << (fpatch->dimensions & 15);
    uint16_t height = 1 << (fpatch->dimensions >> 4);
    if (width < 1 || height < 1) {
        I_Error("M_Load: Patch too small");
    }
    // Verify the size is as promised.
    size_t datasize = height * width;
    if (size < 1 + datasize) {
        I_Error("M_Load: Patch missing data");
    }
    // Allocate patch and copy data over.
    patch->width = width;
    patch->height = height;
    patch->data = Allocate(datasize);
    memcpy(&patch->data[0], &fpatch->data[0], datasize);
    // Free the file data.
    Deallocate(fpatch);
}

static void LoadPatches(uint32_t branch, map_t *map) {
    char *fpatches = ReadMapFile(branch, "patches", &map->numpatches, sizeof(char[8]));
    // Each 8 bytes is a patch name.
    map->patches = Allocate(sizeof(patch_t) * map->numpatches);
    for (size_t i = 0; i < map->numpatches; i++) {
        LoadPatch(&map->patches[i], &fpatches[8 * i]);
    }
    // Free file data.
    Deallocate(fpatches);
}

// Load a flat from a bitmap.
static void LoadFlat(flat_t *flat, const char *name) {
    size_t size;
    uint8_t *fflat = W_ReadLump(
        W_GetNumByName(branchflats, name),
        &size
    );
    if (size < 4096) {
        I_Error("M_Load: Flat missing data");
    }
    // Copy the data.
    memcpy(&flat->data[0], fflat, sizeof(flat->data));
    // Free file data.
    Deallocate(fflat);
}

static void LoadFlats(uint32_t branch, map_t *map) {
    char *fflats = ReadMapFile(branch, "flats", &map->numflats, sizeof(char[8]));
    // Each 8 bytes is a patch name.
    map->flats = Allocate(sizeof(flat_t) * map->numflats);
    for (size_t i = 0; i < map->numflats; i++) {
        LoadFlat(&map->flats[i], &fflats[8 * i]);
    }
    // Free file data.
    Deallocate(fflats);
}

static void LoadVertices(uint32_t branch, map_t *map) {
    file_vertex_t *fvtxs = ReadMapFile(branch, "vertices", &map->numvtxs, sizeof(file_vertex_t));
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

static patch_t *GetPatchById(map_t *map, uint8_t id) {
    if (id == 0) {
        // No patch.
        return NULL;
    }
    // Check patch array.
    id -= 1;
    if (id >= map->numpatches) {
        I_Error("M_Load: Patch index %d is out of range", id);
    }
    return &map->patches[id];
}

static flat_t *GetFlatById(map_t *map, uint8_t id) {
    if (id == 0) {
        // No flat. Sky will be used instead.
        return NULL;
    }
    // Check flat array.
    id -= 1;
    if (id >= map->numflats) {
        I_Error("M_Load: Flat index %d is out of range", id);
    }
    return &map->flats[id];
}

static void LoadWalls(uint32_t branch, map_t *map) {
    file_wall_t *fwalls = ReadMapFile(branch, "walls", &map->numwalls, sizeof(file_wall_t));
    // Allocate walls.
    map->walls = Allocate(sizeof(wall_t) * map->numwalls);
    // Convert walls. Not all data is validated until sectors are converted.
    for (size_t i = 0; i < map->numwalls; i++) {
        file_wall_t *fwall = &fwalls[i];
        wall_t *wall = &map->walls[i];
        // Check bounds of wall vertex.
        if (fwall->vertex >= map->numvtxs) {
            I_Error("M_Load: Vertices of wall %d are out of bounds", i);
        }
        // Store vertex 1. Vertex 2 cannot be set until sectors are converted.
        wall->v1 = &map->vtxs[fwall->vertex];
        // Store the portal index directly into the portal pointer. The sector
        // conversion routine will finish the conversion.
        wall->portal = (void *) (uintptr_t) fwall->portal;
        // Store the wall patches.
        wall->toppatch = GetPatchById(map, fwall->textop);
        wall->midpatch = GetPatchById(map, fwall->texmid);
        wall->botpatch = GetPatchById(map, fwall->texbot);
        // Store the wall's offsets.
        wall->xoffset = fwall->xoffset;
        wall->yoffset = fwall->yoffset;
    }
    // Free file data.
    Deallocate(fwalls);
}

static void LoadSectors(uint32_t branch, map_t *map) {
    file_sector_t *fscts = ReadMapFile(branch, "sectors", &map->numscts, sizeof(file_sector_t));
    // This error will be obsolete once actors are supported.
    if (map->numscts == 0) {
        I_Error("Map has no sectors.");
    }
    // Allocate sectors.
    map->scts = Allocate(sizeof(sector_t) * map->numscts);
    // Convert sectors.
    for (size_t i = 0; i < map->numscts; i++) {
        file_sector_t *fsector = &fscts[i];
        sector_t *sector = &map->scts[i];
        // Check that the sector is a polygon.
        if (fsector->num_walls < 3) {
            I_Error("M_Load: Sector %d is not a polygon", i);
        }
        if (fsector->ceiling <= fsector->floor) {
            I_Error("M_Load: Sector %d has non-positive vertical space", i);
        }
        sector->floor = fsector->floor;
        sector->ceiling = fsector->ceiling;
        // Check that the wall slice is in bounds.
        size_t wstart = fsector->first_wall;
        size_t wend = wstart + fsector->num_walls;
        if (wend > map->numwalls) {
            I_Error("M_Load: Walls of sector %d are out of bounds", i);
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
                I_Error("M_Load: Wall %d of sector %d has zero length", j, i);
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
                    I_Error("M_Load: Portal index of wall %d of sector %d is out of bounds", j, i);
                }
                wall->portal = &map->scts[portalindex];
            } else {
                wall->portal = NULL;
            }
        }
        // Initialize iterator lists.
        sector->next_seen = NULL;
        sector->next_queue = NULL;
        // Initialize actor list.
        U_ListInit(&sector->actors);
        // Set flats.
        sector->floorflat = GetFlatById(map, fsector->floorflat);
        sector->ceilflat = GetFlatById(map, fsector->ceilflat);
    }
    // Free file data.
    Deallocate(fscts);
}

map_t *M_Load(const char *name) {
    // Allocate map.
    map_t *map = Allocate(sizeof(map_t));
    // Open branch containing map lumps.
    uint32_t branch = W_GetNumByName(W_GetNumByName(ROOTID, "maps"), name);
    // Load the textures.
    LoadPatches(branch, map);
    LoadFlats(branch, map);
    // Load each part of the map.
    LoadVertices(branch, map);
    LoadWalls(branch, map);
    LoadSectors(branch, map);
    return map;
}

void M_Free(map_t *map) {
    Deallocate(map->vtxs);
    Deallocate(map->walls);
    Deallocate(map->scts);
    for (size_t i = 0; i < map->numpatches; i++) {
        Deallocate(map->patches[i].data);
    }
    Deallocate(map->patches);
    Deallocate(map->flats);
    Deallocate(map);
}
