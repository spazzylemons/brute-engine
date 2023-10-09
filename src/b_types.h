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

// A two-dimensional point.
typedef struct {
    float x;
    float y;
} vertex_t;

#endif
