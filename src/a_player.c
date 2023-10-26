#include "a_classes.h"
#include "i_input.h"
#include "u_error.h"
#include "u_math.h"

#include <math.h>

// Deadzone in degrees.
#define DEADZONE 10.0f

// Turn speed multiplier.
#define TURNSPEED 0.005f

// Move the player in crank mode.
static void MovePlayerCrank(actor_t *this, vector_t *delta, buttonmask_t held) {
    float crankangle = I_GetCrankAngle();
    // Only do crank movement if in left half.
    if (crankangle <= 180.0f) {
        // Deadzone of ten degrees either way.
        float anglechange;
        if (crankangle < 90.0f - DEADZONE) {
            anglechange = ((90.0f - DEADZONE) - crankangle) * TURNSPEED;
        } else if (crankangle > 90.0f + DEADZONE) {
            anglechange = ((90.0f + DEADZONE) - crankangle) * TURNSPEED;
        } else {
            anglechange = 0.0f;
        }
        this->angle = U_AngleAdd(this->angle, anglechange);
    }

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

static void Update(actor_t *this) {
    buttonmask_t held = I_GetHeldButtons();

    // Check crank state and move according to crank.
    vector_t delta;
    if (I_IsCrankDocked()) {
        MovePlayerDPad(this, &delta, held);
    } else {
        MovePlayerCrank(this, &delta, held);
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
    .init = NULL,
    .update = Update,
};
