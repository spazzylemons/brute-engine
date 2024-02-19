#include "i_file.h"
#include "i_system.h"
#include "u_list.h"
#include "w_pack.h"
#include "y_log.h"
#include "z_memory.h"

#include <string.h>

#define BRANCHFLAG 0x80000000U

// TODO this does not have many overflow checks.

// Node in a pack.
typedef struct __attribute__((__packed__)) {
    union {
        char name[MAXNODENAME]; // Name of node. Applies if not the root node.

        struct {
            char magic[4];  // "PACK" magic number.
            uint32_t count; // Number of nodes in pack.
        };
    };
    uint32_t size;
    uint32_t offset;
} packnode_t;


// The file for the loaded pack.
static file_t *packfile = NULL;
// The node table. First node is the root.
static packnode_t *packnodes = NULL;

bool W_Open(const char *filename) {
    // Can't yet open multiple packs.
    if (packnodes != NULL) {
        return false;
    }
    // Open the file.
    packfile = I_FileOpen(filename, OPEN_RD);
    if (packfile == NULL) {
        return false;
    }
    // Read the root node.
    packnode_t root;
    if (I_FileRead(packfile, &root, sizeof(packnode_t)) != sizeof(packnode_t)) {
        goto fail;
    }
    // Check the magic number.
    if (memcmp(root.magic, "PACK", 4)) {
        goto fail;
    }
    // Check validity of number of files.
    if (root.count == 0) {
        goto fail;
    }
    // Allocate pack with table of needed size.
    packnodes = Allocate((sizeof(packnode_t) * root.count));
    // Copy root node into table.
    memcpy(packnodes, &root, sizeof(packnode_t));
    // Read the rest of the table.
    if (I_FileRead(packfile, &packnodes[1], sizeof(packnode_t) * (root.count - 1)) != sizeof(packnode_t) * (root.count - 1)) {
        Deallocate(packnodes);
        packnodes = NULL;
        goto fail;
    }
    // Looks good to us. If there's any invalid data, we'll check when we get there.
    return true;
fail:
    I_FileClose(packfile);
    packfile = NULL;
    return false;
}

void W_Close(void) {
    if (packnodes != NULL) {
        Deallocate(packnodes);
        I_FileClose(packfile);
    }
}

static packnode_t *GetNodeByNum(uint32_t num) {
    // Check if pack is loaded.
    if (packnodes == NULL) {
        I_Error("No pack loaded");
    }
    // Check if index is valid.
    if (num >= packnodes[0].count) {
        I_Error("Node num %u out of bounds of pack with %u nodes", num, packnodes[0].count);
    }
    // Return node.
    return &packnodes[num];
}

static bool IsBranch(const packnode_t *node) {
    return !!(node->size & BRANCHFLAG);
}

static void AssertBranch(const packnode_t *node) {
    if (!IsBranch(node)) {
        I_Error("Attempted to use lump node like a branch");
    }
}

static void AssertLump(const packnode_t *node) {
    if (IsBranch(node)) {
        I_Error("Attempted to use branch node like a lump");
    }
}

uint32_t W_CheckNumByName(uint32_t parent, const char *name) {
    // Get parent node.
    const packnode_t *parentnode = GetNodeByNum(parent);
    // Assert that it's a branch.
    AssertBranch(parentnode);
    // Go through its children.
    uint32_t parentsize = parentnode->size & ~BRANCHFLAG;
    for (uint32_t i = 0; i < parentsize; i++) {
        const packnode_t *childnode = GetNodeByNum(parentnode->offset + i);
        if (!strncmp(childnode->name, name, 8)) {
            // Name matches.
            return parentnode->offset + i;
        }
    }
    // Name does not match.
    return 0;
}

uint32_t W_GetNumByName(uint32_t parent, const char *name) {
    uint32_t result = W_CheckNumByName(parent, name);
    if (result == 0) {
        I_Error("Pack does not have required node");
    }
    return result;
}

void *W_ReadLump(uint32_t num, size_t *size) {
    // Get the node.
    const packnode_t *node = GetNodeByNum(num);
    // Assert that it's a lump.
    AssertLump(node);
    // Seek to its offset.
    I_FileSeek(packfile, node->offset, SEEK_SET);
    // Allocate space for the contents.
    void *result = Allocate(node->size);
    // Read the contents of the file.
    if (I_FileRead(packfile, result, node->size) != node->size) {
        I_Error("Failed to read lump node");
    }
    // Write the size to the pointer.
    if (size != NULL) {
        *size = node->size;
    }
    return result;
}

void W_IterInit(branchiter_t *iter, uint32_t parent) {
    // Get parent node.
    const packnode_t *parentnode = GetNodeByNum(parent);
    // Assert that it's a branch.
    AssertBranch(parentnode);
    // Get its children and size.
    iter->remaining = parentnode->size & ~BRANCHFLAG;
    iter->next = parentnode->offset;
}

uint32_t W_IterNext(branchiter_t *iter, char *name) {
    // If remaining is 0, return 0.
    if (iter->remaining == 0) {
        return 0;
    }
    // If name requested, get the name.
    if (name != NULL) {
        memcpy(name, GetNodeByNum(iter->next)->name, MAXNODENAME);
    }
    // Get next, advance, and return.
    --iter->remaining;
    return iter->next++;
}
