#ifndef BRUTE_A_ACTOR_H
#define BRUTE_A_ACTOR_H

/**
 * Actors are all objects in the map from the player to enemies to projectiles.
 */

#include "types.h"
#include "map/defs.h"
#include "util/list.h"

#define ACTOR_CLASS "brute.classes.actor"

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
    // Node in list of actors per sector.
    list_t sectorlist;
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

// Spawn an actor.
actor_t *actor_spawn(const vector_t *pos, const map_t *map);

// Destroy an actor.
void actor_despawn(actor_t *this);

// Common code for applying horizontal velocity.
void actor_apply_velocity(actor_t *this);

// Common code for applying gravity.
void actor_apply_gravity(actor_t *this);

// Update the actor's current sector.
void actor_update_sector(actor_t *this);

actor_t *get_actor_pointer(void);

void register_actor_class(void);

#endif
