#ifndef BRUTE_B_TYPES_H
#define BRUTE_B_TYPES_H

/**
 * Common types used across the Brute engine.
 */

#include <stddef.h>
#include <stdint.h>

// An angle. A full turn is 65536 angle units.
typedef uint16_t angle_t;

// Various useful angles.
#define DEG_0   ((angle_t) 0x0000)
#define DEG_90  ((angle_t) 0x4000)
#define DEG_180 ((angle_t) 0x8000)
#define DEG_270 ((angle_t) 0xc000)

// A 16.16 signed fixed-point integer.
typedef int32_t fixed_t;

// The number of bits in the fractional component of fixed_t.
#define FRACBITS 16

// A two-dimensional point.
typedef struct {
    fixed_t x;
    fixed_t y;
} vertex_t;

#endif
