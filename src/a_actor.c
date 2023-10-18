#include "a_actor.h"
#include "b_core.h"

// The list of actors.
static actorlist_t actor_list = { &actor_list, &actor_list };

// Link an actor to the list.
static void ActorLink(actor_t *actor) {
    actor->list.next = actor_list.next;
    actor->list.prev = &actor_list;
    actor_list.next->prev = &actor->list;
    actor_list.next = &actor->list;
}

// Unlink an actor from the list and deallocate it.
static void ActorDestroy(actor_t *actor) {
    actorlist_t *prev = actor->list.prev;
    actorlist_t *next = actor->list.next;
    actor->list.prev->next = next;
    actor->list.next->prev = prev;
    Deallocate(actor);
}

void A_ActorClear(void) {
    actorlist_t *list = &actor_list;
    while (list->next != &actor_list) {
        list = list->next;
        Deallocate((actor_t *) list);
    }
    actor_list.next = &actor_list;
    actor_list.prev = &actor_list;
}

actor_t *A_ActorSpawn(
    const actorclass_t *class,
    const vector_t *pos,
    float angle,
    float zpos,
    sector_t *sector
) {
    // Allocate actor.
    actor_t *actor = Allocate(class->size);
    if (actor == NULL) {
        return NULL;
    }
    // Add to linked list.
    // Fill in fields.
    U_VecCopy(&actor->pos, pos);
    actor->angle = angle;
    actor->zpos = zpos;
    actor->sector = sector;
    actor->class = class;
    // Initialize actor.
    if (class->init != NULL) {
        class->init(actor);
    }
    // Return actor.
    return actor;
}
