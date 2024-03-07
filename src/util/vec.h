#ifndef BRUTE_U_VEC_H
#define BRUTE_U_VEC_H

/**
 * Support for 2-dimensional vectors.
 */

// A two-dimensional point.
typedef struct {
    float x;
    float y;
} vector_t;

// Copy a vector to another vector.
void U_VecCopy(vector_t *dst, const vector_t *src);

// Add a vector to another vector.
void U_VecAdd(vector_t *dst, const vector_t *src);

// Subtract a vector from another vector.
void U_VecSub(vector_t *dst, const vector_t *src);

// Scale a vector by a given factor.
void U_VecScale(vector_t *vtx, float scale);

// Add a vector to another vector with a given scale.
void U_VecScaledAdd(vector_t *dst, const vector_t *src, float scale);

// Get the dot product of two vectors.
float U_VecDot(const vector_t *a, const vector_t *b);

// Get the squared length of a vector.
float U_VecLenSq(const vector_t *vtx);

// Get the squared distance between two vectors.
float U_VecDistSq(const vector_t *a, const vector_t *b);

// Normalize a vector.
void U_VecNormalize(vector_t *vtx);

#endif
