#include "a_classes.h"
#include "b_core.h"
#include "m_map.h"
#include "u_error.h"
#include "u_math.h"

// Deadzone in degrees.
#define DEADZONE 10.0f

// Turn speed multiplier.
#define TURNSPEED 0.005f

// Move the player in crank mode.
static void MovePlayerCrank(actor_t *this, vector_t *delta, PDButtons held) {
    float crankangle = playdate->system->getCrankAngle();
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

    if (held & (kButtonLeft | kButtonRight | kButtonUp | kButtonDown)) {
        // Check which direction we're moving in.
        float moveangle;
        if (held & kButtonLeft) {
            if (held & kButtonUp) {
                moveangle = DEG_45;
            } else if (held & kButtonDown) {
                moveangle = DEG_135;
            } else {
                moveangle = DEG_90;
            }
        } else if (held & kButtonRight) {
            if (held & kButtonUp) {
                moveangle = DEG_315;
            } else if (held & kButtonDown) {
                moveangle = DEG_225;
            } else {
                moveangle = DEG_270;
            }
        } else if (held & kButtonUp) {
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
static void MovePlayerDPad(actor_t *this, vector_t *delta, PDButtons held) {
    // Check turning.
    if (held & kButtonLeft) {
        this->angle = U_AngleAdd(this->angle, 0.05f);
    } else if (held & kButtonRight) {
        this->angle = U_AngleAdd(this->angle, -0.05f);
    }

    if (held & kButtonUp) {
        delta->x = -sinf(this->angle) * 6.0f;
        delta->y = cosf(this->angle) * 6.0f;
    } else if (held & kButtonDown) {
        delta->x = sinf(this->angle) * 6.0f;
        delta->y = -cosf(this->angle) * 6.0f;
    } else {
        delta->x = 0.0f;
        delta->y = 0.0f;
    }
}

static void Update(actor_t *this) {
    PDButtons held;
    playdate->system->getButtonState(&held, NULL, NULL);

    // Check crank state and move according to crank.
    vector_t delta;
    if (playdate->system->isCrankDocked()) {
        MovePlayerDPad(this, &delta, held);
    } else {
        MovePlayerCrank(this, &delta, held);
    }
    this->sector = M_MoveAndSlide(this->sector, &this->pos, &delta);

    if (held & kButtonA) {
        this->zpos += 4.0f;
    }

    if (held & kButtonB) {
        this->zpos -= 4.0f;
    }
}

const actorclass_t A_ClassPlayer = {
    .size = sizeof(actor_t),
    .init = NULL,
    .update = Update,
};
