#include "render/fixed.h"

#include <math.h>

fixed_t fixed_mul(fixed_t a, fixed_t b) {
    return ((int64_t) a * b) >> FRACBITS;
}

fixed_t fixed_div(fixed_t a, fixed_t b) {
    return ((int64_t) a << FRACBITS) / b;
}

fixed_t float_to_fixed(float f) {
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
