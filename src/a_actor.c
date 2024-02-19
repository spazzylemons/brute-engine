#include "a_actor.h"
#include "m_iter.h"
#include "m_map.h"
#include "z_memory.h"

#include <math.h>

#define GRAVITY -1.0f

#define CLIMB_SPEED 6.0f

// The list of actors.
EMPTY_LIST(actorlist);

void A_ActorClear(void) {
    listiter_t iter;
    U_ListIterInit(&iter, &actorlist);
    actor_t *actor;

    while ((actor = (actor_t *) U_ListIterNext(&iter)) != NULL) {
        U_ListRemove(&actor->list);
        Deallocate(actor);
    }
}

actor_t *A_ActorSpawn(const vector_t *pos, const map_t *map) {
    // Allocate actor.
    actor_t *actor = Allocate(sizeof(actor_t));
    actor->sector = &map->scts[0];
    // Add to linked lists.
    U_ListInsert(&actorlist, &actor->list);
    U_ListInsert(&actor->sector->actors, &actor->slist);
    // Fill in fields.
    U_VecCopy(&actor->pos, pos);
    A_ActorUpdateSector(actor);
    actor->flags = 0;
    actor->vel.x = 0.0f;
    actor->vel.y = 0.0f;
    actor->angle = 0.0f;
    actor->zpos = actor->sector->floor;
    actor->zvel = 0.0f;
    // Return actor.
    return actor;
}

void A_ActorApplyVelocity(actor_t *this) {
    // TODO reduce coupling
    M_MoveAndSlide(this);
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

void A_ActorUpdateSector(actor_t *this) {
    M_SectorIterInit(this->sector);

    // Iterate until we find a sector that contains the actor in its walls.
    sector_t *sector;
    while ((sector = M_SectorIterPop())) {
        if (M_SectorContainsPoint(sector, &this->pos)) {
            // This sector contains our point.
            break;
        }

        // Find any portals and follow them.
        for (size_t i = 0; i < sector->num_walls; i++) {
            const wall_t *wall = &sector->walls[i];
            if (wall->portal != NULL) {
                M_SectorIterPush(wall->portal);
            }
        }
    }
    M_SectorIterCleanup();

    // If the actor somehow isn't in any sector, keep it in the sector it was
    // last seen in, as a failsafe. Otherwise, update the value of its sector
    // field, and link it to the list of the new sector. Also, don't do anything
    // if the sector didn't change.
    if (sector != NULL && sector != this->sector) {
        // Unlink from previous sector.
        U_ListRemove(&this->slist);
        // Link to new sector.
        U_ListInsert(&sector->actors, &this->slist);
        // Update sector field.
        this->sector = sector;
    }
}
