#include "a_classes.h"
#include "b_core.h"
#include "m_map.h"
#include "u_math.h"

static void Update(actor_t *this) {
    PDButtons held, pressed;
    playdate->system->getButtonState(&held, &pressed, NULL);

    if (held & kButtonLeft) {
        this->angle = U_AngleAdd(this->angle, 0.05f);
    }
    if (held & kButtonRight) {
        this->angle = U_AngleAdd(this->angle, -0.05f);
    }
    vector_t delta;
    if (held & kButtonUp) {
        delta.x = -sinf(this->angle) * 4.0f;
        delta.y = cosf(this->angle) * 4.0f;
    } else if (held & kButtonDown) {
        delta.x = sinf(this->angle) * 4.0f;
        delta.y = -cosf(this->angle) * 4.0f;
    } else {
        delta.x = 0.0f;
        delta.y = 0.0f;
    }

    this->sector = M_MoveAndSlide(this->sector, &this->pos, &delta);
}

const actorclass_t A_ClassPlayer = {
    .size = sizeof(actor_t),
    .init = NULL,
    .update = Update,
};
