#ifndef BRUTE_U_LIST_H
#define BRUTE_U_LIST_H

/**
 * Generic intrusive doubly linked list. Inspired by Wayland's wl_list utility
 * functions.
 */

// List node and/or object.
typedef struct list_s {
    // The previous element in the list.
    struct list_s *prev;
    // The next element in the list.
    struct list_s *next;
} list_t;

// List iterator.
typedef struct {
    // Head of list.
    list_t *head;
    // Next list element to iterate on.
    list_t *next;
} listiter_t;

// Initialize a list.
void U_ListInit(list_t *list);

// Insert an element into a list. The element must not be part of a list already.
void U_ListInsert(list_t *list, list_t *element);

// Remove an element from its list.
void U_ListRemove(list_t *element);

// Initialize a list iterator.
void U_ListIterInit(listiter_t *iter, list_t *list);

// Get the next value in the list iterator. Returns NULL when at the end.
// It is safe to remove this element from the list.
list_t *U_ListIterNext(listiter_t *iter);

#endif
