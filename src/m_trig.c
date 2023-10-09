#include "m_tables.h"
#include "m_trig.h"

fixed_t M_Sine(angle_t angle) {
    uint16_t index = angle >> 6;
    fixed_t a, b;
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
    return (a * (64 - x) + b * x) >> 6;
}

fixed_t M_Cosine(angle_t angle) {
    return M_Sine(angle + DEG_90);
}
