#include "util/list.h"

#include <stddef.h>

void list_init(list_t *list) {
    list->prev = list;
    list->next = list;
}

void list_insert(list_t *list, list_t *element) {
    element->prev = list;
    element->next = list->next;
    list->next = element;
    element->next->prev = element;
}

void list_remove(list_t *element) {
    element->prev->next = element->next;
    element->next->prev = element->prev;
    element->next = NULL;
    element->prev = NULL;
}

void listiter_init(listiter_t *iter, const list_t *list) {
    iter->head = list;
    iter->next = list->next;
}

list_t *listiter_next(listiter_t *iter) {
    list_t *result = iter->next;
    // Check if at end.
    if (result == iter->head) {
        return NULL;
    }
    // Store next element in iterator.
    iter->next = result->next;
    // Return list element.
    return result;
}
