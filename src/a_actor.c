#include "a_actor.h"
#include "m_iter.h"
#include "m_map.h"
#include "z_memory.h"

#include <math.h>

#define GRAVITY -1.0f

#define CLIMB_SPEED 6.0f

// The list of actors.
static EMPTY_LIST(actorlist);

void A_ActorClear(void) {
    listiter_t iter;
    U_ListIterInit(&iter, &actorlist);
    actor_t *actor;

    while ((actor = (actor_t *) U_ListIterNext(&iter)) != NULL) {
        U_ListRemove(&actor->list);
        Deallocate(actor);
    }
}

actor_t *A_ActorSpawn(
    const actorclass_t *class,
    const vector_t *pos,
    float angle,
    sector_t *sector
) {
    // Allocate actor.
    actor_t *actor = Allocate(class->size);
    // Add to linked list.
    U_ListInsert(&actorlist, &actor->list);
    // Fill in fields.
    U_VecCopy(&actor->pos, pos);
    actor->vel.x = 0.0f;
    actor->vel.y = 0.0f;
    actor->angle = angle;
    actor->zpos = sector->floor;
    actor->zvel = 0.0f;
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

void A_ActorApplyVelocity(actor_t *this) {
    // Move and slide on map.
    this->sector = M_MoveAndSlide(this->sector, &this->pos, this->zpos, &this->vel);
}

void A_ActorApplyGravity(actor_t *this) {
    this->zpos += this->zvel;
    // Climb up smoothly if lower than floor.
    M_SectorIterInit(this->sector);
    sector_t *sector;
    float target_zpos = -INFINITY;
    while ((sector = M_SectorIterPop()) != NULL) {
        if (sector->floor > target_zpos && sector->floor < this->zpos + MAX_STAIR_HEIGHT) {
            target_zpos = sector->floor;
        }

        for (size_t i = 0; i < sector->num_walls; i++) {
            const wall_t *wall = &sector->walls[i];
            if (wall->portal != NULL && 
                M_SectorCanBePushed(wall->portal) &&
                M_SectorContainsCircle(wall->portal, &this->pos, OBJ_RADIUS)) {
                M_SectorIterPush(wall->portal);
            }
        }
    }
    M_SectorIterCleanup();
    if (this->zpos <= target_zpos) {
        this->zvel = 0.0f;
        if (this->zpos < target_zpos) {
            this->zpos += CLIMB_SPEED;
            if (this->zpos > target_zpos) {
                this->zpos = target_zpos;
            }
        }
    } else {
        this->zvel += GRAVITY;
    }
}
