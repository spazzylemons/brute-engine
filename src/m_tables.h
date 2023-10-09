#ifndef BRUTE_M_TABLES_H
#define BRUTE_M_TABLES_H

/**
 * Lookup tables for various purposes.
 */

#include "b_types.h"

// Lookup table for sine and cosine. Despite its size, it is never more than one angle unit off from
// the true value, thanks to interpolation.
extern fixed_t M_SineTable[257];

#endif
