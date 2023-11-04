#ifndef BRUTE_W_PACK_H
#define BRUTE_W_PACK_H

/**
 * Interface to asset pack system. The pack system is similar to WADs, but with
 * the ability to represent a hierarchical directory structure. All entries in
 * a pack are called nodes. Directories are branch nodes, and files are lump
 * nodes. Currently, only one pack can be loaded at a time.
 */

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

// Maximum length of node name.
#define MAXNODENAME 8

// The ID of the root node.
#define ROOTID 0

// Branch iterator.
typedef struct {
    uint32_t remaining; // Number of remaining nodes.
    uint32_t next;      // Next node to load.
} branchiter_t;

// Open a pack. Returns true on success.
bool W_Open(const char *filename);

// Close the loaded pack.
void W_Close(void);

// Find a node in a branch by name, or return 0 if not found.
uint32_t W_CheckNumByName(uint32_t parent, const char *name);

// Find a node in a branch by name, which must exist.
uint32_t W_GetNumByName(uint32_t parent, const char *name);

// Read a lump node into memory.
void *W_ReadLump(uint32_t num, size_t *size);

// Initialize a branch iterator.
void W_IterInit(branchiter_t *iter, uint32_t parent);

// Get next node in branch, or 0 if at end. Optionally retrieve name.
uint32_t W_IterNext(branchiter_t *iter, char name[static MAXNODENAME]);

#endif
