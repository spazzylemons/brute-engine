#ifndef BRUTE_U_MATH_H
#define BRUTE_U_MATH_H

// Various mathematical utilities and constants.

// Common multiples and fractions of Pi.
#define PI   3.14159265f // 0x40490fdb
#define TAU  6.2831853f  // 0x40c90fdb
#define PI_2 1.57079632f // 0x3fc90fdb
#define PI_4 0.78539816f // 0x3f490fdb

// Eight-direction radian angles.
#define DEG_0   0.0f
#define DEG_45  PI_4
#define DEG_90  PI_2
#define DEG_135 2.3561944f // 0x4016cbe4
#define DEG_180 PI
#define DEG_225 (-DEG_135)
#define DEG_270 (-DEG_90)
#define DEG_315 (-DEG_45)

// Add two angles and wrap the result.
float U_AngleAdd(float a, float b);

#endif
