#ifndef BRUTE_A_ACTOR_H
#define BRUTE_A_ACTOR_H

/**
 * Actors are all objects in the map from the player to enemies to projectiles.
 */

#include "b_types.h"
#include "m_defs.h"
#include "u_list.h"

typedef enum {
    ACTOR_NORENDER = (1 << 0), // Don't render this actor.
} actorflags_t;

// A sprite type.
typedef enum {
    SPR_TEST,
    NUMSPRITES,
} spritetype_t;

// An actor. This is the base of all actor objects, which can derive from this
// struct by including it as their first member.
typedef struct {
    // Node in list of actors.
    list_t list;
    // Node in list of actors per sector.
    list_t slist;
    // Various flags.
    actorflags_t flags;
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
} actor_t;

extern list_t actorlist;

// Delete all actors.
void A_ActorClear(void);

// Spawn an actor.
actor_t *A_ActorSpawn(const vector_t *pos, const map_t *map);

// Common code for applying horizontal velocity.
void A_ActorApplyVelocity(actor_t *this);

// Common code for applying gravity.
void A_ActorApplyGravity(actor_t *this);

// Update the actor's current sector.
void A_ActorUpdateSector(actor_t *this);

#endif
