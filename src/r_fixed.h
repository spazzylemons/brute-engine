#ifndef BRUTE_R_FIXED_H
#define BRUTE_R_FIXED_H

/**
 * Fixed-point math for rendering routines.
 */

#include <limits.h>
#include <stdint.h>

// Fixed-point integer.
typedef int32_t fixed_t;

// Number of fractional bits in a fixed-point number.
#define FRACBITS 12

// Number of integral bits in a fixed-point number.
#define INTBITS ((CHAR_BIT * sizeof(fixed_t)) - FRACBITS)

// Multiply two fixed-point integers.
fixed_t R_FixedMul(fixed_t a, fixed_t b);

// Divide two fixed-point integers.
fixed_t R_FixedDiv(fixed_t a, fixed_t b);

// Convert a float to a fixed-point number.
fixed_t R_FloatToFixed(float f);

#endif
