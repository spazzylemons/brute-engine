#include "a_actor.h"
#include "b_core.h"

// The list of actors.
static list_t actorlist = { &actorlist, &actorlist };

void A_ActorClear(void) {
    listiter_t iter;
    U_ListIterInit(&iter, &actorlist);
    actor_t *actor;

    while ((actor = (actor_t *) U_ListIterNext(&iter)) != NULL) {
        Deallocate(actor);
    }
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
    // Add to linked list.
    U_ListInsert(&actorlist, &actor->list);
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

void A_ActorUpdate(void) {
    listiter_t iter;
    U_ListIterInit(&iter, &actorlist);
    actor_t *actor;

    while ((actor = (actor_t *) U_ListIterNext(&iter)) != NULL) {
        if (actor->class->update != NULL) {
            actor->class->update(actor);
        }
    }
}
