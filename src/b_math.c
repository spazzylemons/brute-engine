#include "b_math.h"

#include <math.h>

void B_VtxCopy(vertex_t *dst, const vertex_t *src) {
    dst->x = src->x;
    dst->y = src->y;
}

void B_VtxAdd(vertex_t *dst, const vertex_t *src) {
    dst->x += src->x;
    dst->y += src->y;
}

void B_VtxSub(vertex_t *dst, const vertex_t *src) {
    dst->x -= src->x;
    dst->y -= src->y;
}

void B_VtxScale(vertex_t *vtx, float scale) {
    vtx->x *= scale;
    vtx->y *= scale;
}

void B_VtxScaledAdd(vertex_t *dst, const vertex_t *src, float scale) {
    dst->x = fmaf(src->x, scale, dst->x);
    dst->y = fmaf(src->y, scale, dst->y);
}

float B_VtxDot(const vertex_t *a, const vertex_t *b) {
    return a->x * b->x + a->y * b->y;
}

float B_VtxLenSq(const vertex_t *vtx) {
    return B_VtxDot(vtx, vtx);
}

float B_VtxDistSq(const vertex_t *a, const vertex_t *b) {
    vertex_t delta;
    B_VtxCopy(&delta, b);
    B_VtxSub(&delta, a);
    return B_VtxLenSq(&delta);
}

void B_VtxNormalize(vertex_t *vtx) {
    float lensq = B_VtxLenSq(vtx);
    if (lensq > 0.0f) {
        lensq = 1.0f / sqrtf(lensq);
        vtx->x *= lensq;
        vtx->y *= lensq;
    }
}
