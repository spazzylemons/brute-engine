#include "r_fixed.h"

#include <math.h>

fixed_t R_FixedMul(fixed_t a, fixed_t b) {
    return ((int64_t) a * b) >> FRACBITS;
}

fixed_t R_FixedDiv(fixed_t a, fixed_t b) {
    return ((int64_t) a << FRACBITS) / b;
}

fixed_t R_FloatToFixed(float f) {
    // To avoid loss of precision, we split up the float and convert separately.
    float integral;
    float fractional = modff(f, &integral);
    // Convert the integer part.
    fixed_t result = integral;
    result <<= FRACBITS;
    // Convert the fractional part.
    result += floorf(fractional * (1 << FRACBITS));
    return result;
}
