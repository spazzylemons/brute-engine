#include "m_iter.h"

#include <stddef.h> // for NULL

void M_SectorIterNew(sectoriter_t *iter, sector_t *first) {
    iter->root = first;
    iter->seen = first;
    iter->head = first;
    iter->tail = first;
}

void M_SectorIterPush(sectoriter_t *iter, sector_t *sector) {
    if (sector != iter->root && sector->next_seen == NULL) {
        sector->next_seen = iter->seen;
        iter->seen = sector;

        if (iter->tail == NULL) {
            iter->head = sector;
            iter->tail = sector;
        } else {
            iter->tail->next_queue = sector;
            iter->tail = sector;
        }
    }
}

sector_t *M_SectorIterPop(sectoriter_t *iter) {
    sector_t *result = iter->head;
    if (result != NULL) {
        iter->head = result->next_queue;
        if (iter->head == NULL) {
            iter->tail = NULL;
        }
    }
    return result;
}

void M_SectorIterCleanup(sectoriter_t *iter) {
    while (iter->seen != NULL) {
        sector_t *sector = iter->seen;
        iter->seen = sector->next_seen;
        sector->next_seen = NULL;
        sector->next_queue = NULL;
    }
}
