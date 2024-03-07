#include "map/iter.h"

#include <stddef.h> // for NULL

static sector_t *root; // The first sector iterated over.
static sector_t *seen; // The set of used sectors.
static sector_t *head; // The head of the queue.
static sector_t *tail; // The tail of the queue.

void sector_iter_init(sector_t *first) {
    root = first;
    seen = first;
    head = first;
    tail = first;
}

void sector_iter_push(sector_t *sector) {
    if (sector_can_be_pushed(sector)) {
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

sector_t *sector_iter_pop(void) {
    sector_t *result = head;
    if (result != NULL) {
        head = result->next_queue;
        if (head == NULL) {
            tail = NULL;
        }
    }
    return result;
}

void sector_iter_cleanup(void) {
    while (seen != NULL) {
        sector_t *sector = seen;
        seen = sector->next_seen;
        sector->next_seen = NULL;
        sector->next_queue = NULL;
    }
}

bool sector_can_be_pushed(sector_t *sector) {
    return sector != root && sector->next_seen == NULL;
}
