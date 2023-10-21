#include "u_math.h"

#include <math.h>

float U_AngleAdd(float a, float b) {
    float result = fmodf(a + b, TAU);
	// Wrap from -Pi to +Pi.
	if (result > PI) {
		result -= TAU;
	} else if (result <= -PI) {
		result += TAU;
	}
    return result;
}
