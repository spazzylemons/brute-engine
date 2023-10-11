#ifndef BRUTE_B_MATH_H
#define BRUTE_B_MATH_H

/**
 * Useful math routines.
 */

#include "b_types.h"

// Copy a vertex to another vertex.
void B_VtxCopy(vertex_t *dst, const vertex_t *src);

// Add a vertex to another vertex.
void B_VtxAdd(vertex_t *dst, const vertex_t *src);

// Subtract a vertex from another vertex.
void B_VtxSub(vertex_t *dst, const vertex_t *src);

// Scale a vertex by a given factor.
void B_VtxScale(vertex_t *vtx, float scale);

// Add a vertex to another vertex with a given scale.
void B_VtxScaledAdd(vertex_t *dst, const vertex_t *src, float scale);

// Get the dot product of two vertices.
float B_VtxDot(const vertex_t *a, const vertex_t *b);

// Get the squared length of a vertex.
float B_VtxLenSq(const vertex_t *vtx);

// Get the squared distance between two vertices.
float B_VtxDistSq(const vertex_t *a, const vertex_t *b);

// Normalize a vertex.
void B_VtxNormalize(vertex_t *vtx);

#endif
