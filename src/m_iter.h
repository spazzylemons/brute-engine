#ifndef BRUTE_M_ITER_H
#define BRUTE_M_ITER_H

/**
 * Breadth-first sector iteration.
 */

#include "m_defs.h"

// A structure for iterating over sectors without duplicates.
// Note that this uses fields within the sectors, so it should not be used
// recursively on the same map.
typedef struct {
    // The first sector iterated over.
    sector_t *root;
    // The set of used sectors.
    sector_t *seen;
    // The head of the queue.
    sector_t *head;
    // The tail of the queue.
    sector_t *tail;
} sectoriter_t;

// Initialize a sector iterator.
void M_SectorIterNew(sectoriter_t *iter, sector_t *first);

// Push a sector to a sector iterator's queue.
void M_SectorIterPush(sectoriter_t *iter, sector_t *sector);

// Pop a sector off of the sector iterator's queue. Returns NULL if empty.
sector_t *M_SectorIterPop(sectoriter_t *iter);

// Clean up links in all iterated sectors.
void M_SectorIterCleanup(sectoriter_t *iter);

#endif
