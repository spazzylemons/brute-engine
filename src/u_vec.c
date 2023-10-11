#include "u_vec.h"

#include <math.h>

void U_VecCopy(vector_t *dst, const vector_t *src) {
    dst->x = src->x;
    dst->y = src->y;
}

void U_VecAdd(vector_t *dst, const vector_t *src) {
    dst->x += src->x;
    dst->y += src->y;
}

void U_VecSub(vector_t *dst, const vector_t *src) {
    dst->x -= src->x;
    dst->y -= src->y;
}

void U_VecScale(vector_t *vtx, float scale) {
    vtx->x *= scale;
    vtx->y *= scale;
}

void U_VecScaledAdd(vector_t *dst, const vector_t *src, float scale) {
    dst->x = fmaf(src->x, scale, dst->x);
    dst->y = fmaf(src->y, scale, dst->y);
}

float U_VecDot(const vector_t *a, const vector_t *b) {
    return a->x * b->x + a->y * b->y;
}

float U_VecLenSq(const vector_t *vtx) {
    return U_VecDot(vtx, vtx);
}

float U_VecDistSq(const vector_t *a, const vector_t *b) {
    vector_t delta;
    U_VecCopy(&delta, b);
    U_VecSub(&delta, a);
    return U_VecLenSq(&delta);
}

void U_VecNormalize(vector_t *vtx) {
    float lensq = U_VecLenSq(vtx);
    if (lensq > 0.0f) {
        lensq = 1.0f / sqrtf(lensq);
        vtx->x *= lensq;
        vtx->y *= lensq;
    }
}
