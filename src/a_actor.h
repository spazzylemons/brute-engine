#ifndef BRUTE_A_ACTOR_H
#define BRUTE_A_ACTOR_H

/**
 * Actors are all objects in the map from the player to enemies to projectiles.
 */

#include "b_types.h"
#include "m_defs.h"
#include "u_list.h"

// An actor. This is the base of all actor objects, which can derive from this
// struct by including it as their first member.
typedef struct {
    // Node in list of actors.
    list_t list;
    // The actor's position.
    vector_t pos;
    // The actor's horizontal velocity.
    vector_t vel;
    // The actor's vertical position.
    float zpos;
    // The actor's vertical velocity.
    float zvel;
    // The actor's rotation.
    float angle;
    // The sector the actor was last seen in.
    sector_t *sector;
    // The actor's class.
    const struct actorclass_s *class;
} actor_t;

// An actor class. Describes how an actor should act.
typedef struct actorclass_s {
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

// Common code for applying horizontal velocity.
void A_ActorApplyVelocity(actor_t *this);

// Common code for applying gravity.
void A_ActorApplyGravity(actor_t *this);

// Update all actors.
void A_ActorUpdate(void);

#endif
