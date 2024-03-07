#ifndef BRUTE_M_ITER_H
#define BRUTE_M_ITER_H

/**
 * Breadth-first sector iteration. Be careful as only one iteration can
 * occur at a time, and cleanup must be called before another iteration starts.
 */

#include "map/defs.h"

// Initialize a sector iterator.
void sector_iter_init(sector_t *first);

// Push a sector to a sector iterator's queue.
void sector_iter_push(sector_t *sector);

// Pop a sector off of the sector iterator's queue. Returns NULL if empty.
sector_t *sector_iter_pop(void);

// Clean up links in all iterated sectors.
void sector_iter_cleanup(void);

// Return true if sector can be pushed to the iterator.
bool sector_can_be_pushed(sector_t *sector);

#endif
