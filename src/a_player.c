#include "a_classes.h"
#include "i_input.h"
#include "u_math.h"

#include <math.h>

// Turn speed multiplier.
#define TURNSPEED 0.005f

extern actor_t *fortesting;

// Move the player in crank mode.
static void MovePlayerCrank(actor_t *this, vector_t *delta, buttonmask_t held) {
    this->angle = U_AngleAdd(this->angle, I_GetAnalogStrength() * TURNSPEED);

    if (held & (BTN_L | BTN_R | BTN_U | BTN_D)) {
        // Check which direction we're moving in.
        float moveangle;
        if (held & BTN_L) {
            if (held & BTN_U) {
                moveangle = DEG_45;
            } else if (held & BTN_D) {
                moveangle = DEG_135;
            } else {
                moveangle = DEG_90;
            }
        } else if (held & BTN_R) {
            if (held & BTN_U) {
                moveangle = DEG_315;
            } else if (held & BTN_D) {
                moveangle = DEG_225;
            } else {
                moveangle = DEG_270;
            }
        } else if (held & BTN_U) {
            moveangle = DEG_0;
        } else {
            moveangle = DEG_180;
        }
        // Add our angle to the movement angle.
        moveangle = U_AngleAdd(moveangle, this->angle);
        // Calculate the movement vector.
        delta->x = -sinf(moveangle) * 6.0f;
        delta->y = cosf(moveangle) * 6.0f;
    } else {
        delta->x = 0.0f;
        delta->y = 0.0f;
    }
}

// Move the player in D-pad only mode.
static void MovePlayerDPad(actor_t *this, vector_t *delta, buttonmask_t held) {
    // Check turning.
    if (held & BTN_L) {
        this->angle = U_AngleAdd(this->angle, 0.05f);
    } else if (held & BTN_R) {
        this->angle = U_AngleAdd(this->angle, -0.05f);
    }

    if (held & BTN_U) {
        delta->x = -sinf(this->angle) * 6.0f;
        delta->y = cosf(this->angle) * 6.0f;
    } else if (held & BTN_D) {
        delta->x = sinf(this->angle) * 6.0f;
        delta->y = -cosf(this->angle) * 6.0f;
    } else {
        delta->x = 0.0f;
        delta->y = 0.0f;
    }
}

static void Init(actor_t *this) {
    this->flags |= ACTOR_NORENDER;
}

static void Update(actor_t *this) {
    buttonmask_t held = I_GetHeldButtons();

    // Check crank state and move according to crank.
    vector_t delta;
    if (I_HasAnalogInput()) {
        MovePlayerCrank(this, &delta, held);
    } else {
        MovePlayerDPad(this, &delta, held);
    }

    if (held & BTN_A) {
        U_VecCopy(&fortesting->pos, &this->pos);
        A_ActorUpdateSector(fortesting);
    }

    // Gradually approach target velocity.
    U_VecScaledAdd(&this->vel, &delta, 0.25f);
    U_VecScale(&this->vel, 0.8f);

    // Physics step.
    A_ActorApplyVelocity(this);
    A_ActorApplyGravity(this);
}

const actorclass_t A_ClassPlayer = {
    .size = sizeof(actor_t),
    .init = Init,
    .update = Update,
};
