#include "m_fixed.h"

#include "core.h"

fixed_t M_FixedMul(fixed_t a, fixed_t b) {
    return ((int64_t) a * (int64_t) b) >> FRACBITS;
}

fixed_t M_FixedDiv(fixed_t a, fixed_t b) {
    // TODO account for potential overflow similar to how DOOM does it
    return ((int64_t) a << FRACBITS) / (int64_t) b;
}
