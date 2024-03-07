#include "actor/actor.h"
#include "system.h"
#include "map/iter.h"
#include "map/map.h"

#include <math.h>

#define GRAVITY -1.0f

#define CLIMB_SPEED 6.0f

actor_t *actor_spawn(const vector_t *pos, const map_t *map) {
    // Allocate actor.
    actor_t *actor = playdate->system->realloc(NULL, sizeof(actor_t));
    actor->sector = &map->scts[0];
    // Add to linked list.
    list_insert(&actor->sector->actors, &actor->sectorlist);
    // Fill in fields.
    U_VecCopy(&actor->pos, pos);
    actor_update_sector(actor);
    actor->flags = 0;
    actor->vel.x = 0.0f;
    actor->vel.y = 0.0f;
    actor->angle = 0.0f;
    actor->zpos = actor->sector->floor;
    actor->zvel = 0.0f;
    // Return actor.
    return actor;
}

void actor_despawn(actor_t *this) {
    list_remove(&this->sectorlist);
}

void actor_apply_velocity(actor_t *this) {
    // TODO reduce coupling
    M_MoveAndSlide(this);
}

void actor_apply_gravity(actor_t *this) {
    this->zpos += this->zvel;
    // Climb up smoothly if lower than floor.
    sector_iter_init(this->sector);
    sector_t *sector;
    float target_zpos = -INFINITY;
    while ((sector = sector_iter_pop()) != NULL) {
        if (sector->floor > target_zpos && sector->floor < this->zpos + MAX_STAIR_HEIGHT) {
            target_zpos = sector->floor;
        }

        for (size_t i = 0; i < sector->num_walls; i++) {
            const wall_t *wall = &sector->walls[i];
            if (wall->portal != NULL && 
                sector_can_be_pushed(wall->portal) &&
                M_SectorContainsCircle(wall->portal, &this->pos, OBJ_RADIUS)) {
                sector_iter_push(wall->portal);
            }
        }
    }
    sector_iter_cleanup();
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

void actor_update_sector(actor_t *this) {
    sector_iter_init(this->sector);

    // Iterate until we find a sector that contains the actor in its walls.
    sector_t *sector;
    while ((sector = sector_iter_pop())) {
        if (M_SectorContainsPoint(sector, &this->pos)) {
            // This sector contains our point.
            break;
        }

        // Find any portals and follow them.
        for (size_t i = 0; i < sector->num_walls; i++) {
            const wall_t *wall = &sector->walls[i];
            if (wall->portal != NULL) {
                sector_iter_push(wall->portal);
            }
        }
    }
    sector_iter_cleanup();

    // If the actor somehow isn't in any sector, keep it in the sector it was
    // last seen in, as a failsafe. Otherwise, update the value of its sector
    // field, and link it to the list of the new sector. Also, don't do anything
    // if the sector didn't change.
    if (sector != NULL && sector != this->sector) {
        // Unlink from previous sector.
        list_remove(&this->sectorlist);
        // Link to new sector.
        list_insert(&sector->actors, &this->sectorlist);
        // Update sector field.
        this->sector = sector;
    }
}

actor_t *get_actor_pointer(void) {
    actor_t *actor = playdate->lua->getArgObject(1, ACTOR_CLASS, NULL);
    if (actor == NULL)
        playdate->system->error("Invalid actor");
    return actor;
}

static int func_getPos(lua_State *L) {
    actor_t *actor = get_actor_pointer();
    playdate->lua->pushFloat(actor->pos.x);
    playdate->lua->pushFloat(actor->pos.y);
    return 2;
}

static int func_setPos(lua_State *L) {
    actor_t *actor = get_actor_pointer();
    actor->pos.x = playdate->lua->getArgFloat(2);
    actor->pos.y = playdate->lua->getArgFloat(3);
    actor_update_sector(actor);

    return 0;
}

static int func_getVel(lua_State *L) {
    actor_t *actor = get_actor_pointer();
    playdate->lua->pushFloat(actor->vel.x);
    playdate->lua->pushFloat(actor->vel.y);
    return 2;
}

static int func_setVel(lua_State *L) {
    actor_t *actor = get_actor_pointer();
    actor->vel.x = playdate->lua->getArgFloat(2);
    actor->vel.y = playdate->lua->getArgFloat(3);
    return 0;
}

static int func_getZPos(lua_State *L) {
    actor_t *actor = get_actor_pointer();
    playdate->lua->pushFloat(actor->zpos);
    return 1;
}

static int func_setZPos(lua_State *L) {
    actor_t *actor = get_actor_pointer();
    actor->zpos = playdate->lua->getArgFloat(2);
    return 0;
}

static int func_getZVel(lua_State *L) {
    actor_t *actor = get_actor_pointer();
    playdate->lua->pushFloat(actor->zvel);
    return 1;
}

static int func_setZVel(lua_State *L) {
    actor_t *actor = get_actor_pointer();
    actor->zvel = playdate->lua->getArgFloat(2);
    return 0;
}

static int func_getAngle(lua_State *L) {
    actor_t *actor = get_actor_pointer();
    playdate->lua->pushFloat(actor->angle);
    return 1;
}

static int func_setAngle(lua_State *L) {
    actor_t *actor = get_actor_pointer();
    actor->angle = playdate->lua->getArgFloat(2);
    return 0;
}

static int func_applyVelocity(lua_State *L) {
    actor_t *actor = get_actor_pointer();
    actor_apply_velocity(actor);
    return 0;
}

static int func_applyGravity(lua_State *L) {
    actor_t *actor = get_actor_pointer();
    actor_apply_gravity(actor);
    return 0;
}

static int func_despawn(lua_State *L) {
    actor_t *actor = get_actor_pointer();
    actor_despawn(actor);
    return 0;
}

static int func_free(lua_State *L) {
    actor_t *actor = get_actor_pointer();
    playdate->system->realloc(actor, 0);
    return 0;
}

static lua_reg actor_regs[] = {
    { "getPos",        func_getPos },
    { "setPos",        func_setPos },
    { "getVel",        func_getVel },
    { "setVel",        func_setVel },
    { "getZPos",       func_getZPos },
    { "setZPos",       func_setZPos },
    { "getZVel",       func_getZVel },
    { "setZVel",       func_setZVel },
    { "getAngle",      func_getAngle },
    { "setAngle",      func_setAngle },
    { "applyVelocity", func_applyVelocity },
    { "applyGravity",  func_applyGravity },
    { "despawn",       func_despawn },
    { "free",          func_free },
    { NULL, NULL },
};

void register_actor_class(void) {
    playdate->lua->registerClass(ACTOR_CLASS, actor_regs, NULL, 0, NULL);
}
