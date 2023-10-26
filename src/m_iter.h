#ifndef BRUTE_M_ITER_H
#define BRUTE_M_ITER_H

/**
 * Breadth-first sector iteration. Be careful as only one iteration can
 * occur at a time, and cleanup must be called before another iteration starts.
 */

#include "m_defs.h"

// Initialize a sector iterator.
void M_SectorIterInit(sector_t *first);

// Push a sector to a sector iterator's queue.
void M_SectorIterPush(sector_t *sector);

// Pop a sector off of the sector iterator's queue. Returns NULL if empty.
sector_t *M_SectorIterPop(void);

// Clean up links in all iterated sectors.
void M_SectorIterCleanup(void);

// Return true if sector can be pushed to the iterator.
bool M_SectorCanBePushed(sector_t *sector);

#endif
