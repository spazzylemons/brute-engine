#ifndef BRUTE_M_FIXED_H
#define BRUTE_M_FIXED_H

/**
 * Fixed-point multiplication and division. Addition and subtraction is the same
 * as it is for integers.
 */

#include "b_types.h"

fixed_t M_FixedMul(fixed_t a, fixed_t b);

fixed_t M_FixedDiv(fixed_t a, fixed_t b);

#endif
