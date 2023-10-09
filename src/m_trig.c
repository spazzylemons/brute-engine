#include "m_tables.h"
#include "m_trig.h"

float M_Sine(angle_t angle) {
    uint16_t index = angle >> 6;
    float a, b;
    if (index & 0x100) {
        a = M_SineTable[(0xff ^ (0xff & index)) + 1];
        b = M_SineTable[(0xff ^ (0xff & index))];
    } else {
        a = M_SineTable[0xff & index];
        b = M_SineTable[(0xff & index) + 1];
    }
    if (index & 0x200) {
        a = -a;
        b = -b;
    }
    uint16_t x = angle & 63;
    float la = (64 - x) / 64.0f;
    float lb = x / 64.0f;
    return a * la + b * lb;
}

float M_Cosine(angle_t angle) {
    return M_Sine(angle + DEG_90);
}
