#ifndef BRUTE_M_TABLES_H
#define BRUTE_M_TABLES_H

/**
 * Lookup tables for various purposes.
 */

#include "b_types.h"

// Lookup table for sine and cosine. Table is small but reasonably accurate with
// interpolation.
extern float M_SineTable[257];

#endif
