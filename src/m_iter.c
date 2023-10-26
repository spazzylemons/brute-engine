#include "m_iter.h"

#include <stddef.h> // for NULL

static sector_t *root; // The first sector iterated over.
static sector_t *seen; // The set of used sectors.
static sector_t *head; // The head of the queue.
static sector_t *tail; // The tail of the queue.

void M_SectorIterInit(sector_t *first) {
    root = first;
    seen = first;
    head = first;
    tail = first;
}

void M_SectorIterPush(sector_t *sector) {
    if (M_SectorCanBePushed(sector)) {
        sector->next_seen = seen;
        seen = sector;

        if (tail == NULL) {
            head = sector;
            tail = sector;
        } else {
            tail->next_queue = sector;
            tail = sector;
        }
    }
}

sector_t *M_SectorIterPop(void) {
    sector_t *result = head;
    if (result != NULL) {
        head = result->next_queue;
        if (head == NULL) {
            tail = NULL;
        }
    }
    return result;
}

void M_SectorIterCleanup(void) {
    while (seen != NULL) {
        sector_t *sector = seen;
        seen = sector->next_seen;
        sector->next_seen = NULL;
        sector->next_queue = NULL;
    }
}

bool M_SectorCanBePushed(sector_t *sector) {
    return sector != root && sector->next_seen == NULL;
}
