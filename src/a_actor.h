#ifndef BRUTE_A_ACTOR_H
#define BRUTE_A_ACTOR_H

/**
 * Actors are all objects in the map from the player to enemies to projectiles.
 */

#include "b_types.h"
#include "m_defs.h"

// A node in the list of actors.
// TODO if we need later on, we should make a generic circlar DLL impl
typedef struct actorlist_t {
    struct actorlist_t *prev;
    struct actorlist_t *next;
} actorlist_t;

// An actor. This is the base of all actor objects, which can derive from this
// struct by including it as their first member.
typedef struct {
    actorlist_t list;
    // The actor's position.
    vector_t pos;
    // The actor's rotation.
    float angle;
    // The sector the actor was last seen in.
    sector_t *sector;
    // The actor's class.
    const struct actorclass_t *class;
} actor_t;

// An actor class. Describes how an actor should act.
typedef struct actorclass_t {
    // The size of the object to allocate.
    size_t size;

    // Call to initialize the object.
    // pos, angle, and sector are already set.
    // Can be NULL if not applicable.
    void (*init)(actor_t *this);

    // Call to update the object.
    // Can be NULL if not applicable.
    void (*update)(actor_t *this);
} actorclass_t;

// Delete all actors.
void A_ActorClear(void);

// Spawn an actor.
actor_t *A_ActorSpawn(
    const actorclass_t *class,
    const vector_t *pos,
    float angle,
    sector_t *sector
);

#endif