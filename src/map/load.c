#include "../core.h"
#include "load.h"

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
} file_wall_t;

// Refuse to allocate and read a file larger than this.
#define MAX_SLURP_SIZE 1048576

// Slurp a file's contents. Returns a pointer to the contents and the size of the allocation.
// TODO this should be a utility file elsewhere.
static void *SlurpFile(const char *path, size_t *szp) {
    // How much data are we reading?
    FileStat stat;
    if (playdate->file->stat(path, &stat)) {
        return NULL;
    }
    // Avoid reading if too big.
    if (stat.size > MAX_SLURP_SIZE) {
        return NULL;
    }
    // Allocate the buffer to store that data.
    void *buffer = Allocate(stat.size);
    if (buffer == NULL) {
        return NULL;
    }
    // Read the data from the file.
    SDFile *file = playdate->file->open(path, kFileRead);
    if (file != NULL) {
        int bytes_read = playdate->file->read(file, buffer, stat.size);
        playdate->file->close(file);
        if (bytes_read == (int) stat.size) {
            if (szp != NULL) {
                *szp = stat.size;
            }
            return buffer;
        }
    }
    Deallocate(buffer);
    return NULL;
}

static void *SlurpMap(const char *name, const char *file, size_t *szp, size_t mbsz) {
    // Allocate map path.
    char *path = NULL;
    playdate->system->formatString(&path, "/maps/%s/%s", name, file);
    if (path == NULL) {
        return NULL;
    }
    // Read file.
    void *result = SlurpFile(path, szp);
    Deallocate(path);
    // If successful, divide size by member count.
    if (result != NULL) {
        *szp /= mbsz;
    }
    return result;
}

map_t *M_Load(const char *name) {
    // File data.
    size_t num_vtxs, num_scts, num_walls;
    file_vertex_t *f_vtxs;
    file_sector_t *f_scts;
    file_wall_t *f_walls;
    // Map data.
    vertex_t *vtxs = NULL;
    sector_t *scts = NULL;
    wall_t *walls = NULL;
    map_t *map = NULL;
    // Read the files.
    f_vtxs = SlurpMap(name, "vertices", &num_vtxs, sizeof(file_vertex_t));
    f_scts = SlurpMap(name, "sectors", &num_scts, sizeof(file_sector_t));
    f_walls = SlurpMap(name, "walls", &num_walls, sizeof(file_wall_t));
    // If any failed, stop.
    if (f_vtxs == NULL || f_scts == NULL || f_walls == NULL) {
        goto fail;
    }
    // If no sectors, stop.
    if (num_scts == 0) {
        goto fail;
    }
    // Allocate data structures.
    vtxs = Allocate(sizeof(vertex_t) * num_vtxs);
    scts = Allocate(sizeof(sector_t) * num_scts);
    walls = Allocate(sizeof(wall_t) * num_walls);
    map = Allocate(sizeof(map_t));
    // If any failed, stop.
    if (vtxs == NULL || scts == NULL || walls == NULL) {
        goto fail;
    }
    // Convert vertices.
    for (size_t i = 0; i < num_vtxs; i++) {
        vtxs[i].x = f_vtxs[i].x;
        vtxs[i].y = f_vtxs[i].y;
    }
    // Convert sectors.
    for (size_t i = 0; i < num_scts; i++) {
        file_sector_t *f_sector = &f_scts[i];
        sector_t *sector = &scts[i];
        // Initialize draw lists.
        sector->next_drawn = NULL;
        sector->next_queued = NULL;
        // Must have at least three sides.
        if (f_sector->num_walls < 3) {
            goto fail;
        }
        // Wall array must be in bounds.
        size_t wall_start = f_sector->first_wall;
        size_t wall_end = wall_start + f_sector->num_walls;
        if (wall_end > num_walls) {
            goto fail;
        }
        // Copy wall count and pointer.
        sector->num_walls = f_sector->num_walls;
        sector->walls = &walls[f_sector->first_wall];
        // Convert walls in this sector.
        for (size_t j = 0; j < sector->num_walls; j++) {
            file_wall_t *f_wall = &f_walls[f_sector->first_wall + j];
            file_wall_t *f_next = &f_walls[f_sector->first_wall + (j + 1) % sector->num_walls];
            wall_t *wall = &sector->walls[j];
            // Wall vertex indices must be within bounds.
            if (f_wall->vertex >= num_vtxs || f_next->vertex >= num_vtxs) {
                goto fail;
            }
            wall->v1 = &vtxs[f_wall->vertex];
            wall->v2 = &vtxs[f_next->vertex];
            // Wall portal index must be within bounds.
            if (f_wall->portal != i) {
                if (f_wall->portal >= num_scts) {
                    goto fail;
                }
                wall->portal = &scts[f_wall->portal];
            } else {
                wall->portal = NULL;
            }
        }
    }
    // Free file data.
    Deallocate(f_vtxs);
    Deallocate(f_scts);
    Deallocate(f_walls);
    // Successful conversion. Create and return map object.
    map->vtxs = vtxs;
    map->scts = scts;
    map->walls = walls;
    return map;
fail:
    // Free file data.
    Deallocate(f_vtxs);
    Deallocate(f_scts);
    Deallocate(f_walls);
    // Free vertex data.
    Deallocate(vtxs);
    Deallocate(scts);
    Deallocate(walls);
    Deallocate(map);
    return NULL;
}

void M_Free(map_t *map) {
    Deallocate(map->vtxs);
    Deallocate(map->scts);
    Deallocate(map->walls);
    Deallocate(map);
}
