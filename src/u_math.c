#include "u_math.h"

#include <math.h>

float U_AngleAdd(float a, float b) {
    float result = fmodf(a + b, TAU);
	if (result < 0.0f) {
		result += TAU;
	}
    return result;
}
