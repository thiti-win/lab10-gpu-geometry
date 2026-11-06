#pragma once
#include "gpu.h"
#include <cmath>
struct vec3 {
    float x = 0;
    float y = 0;
    float z = 0;
};
using Color = vec3;
HD inline vec3 operator + (vec3 a, vec3 b) {
    return {a.x + b.x, a.y + b.y, a.z + b.z};
}
HD inline vec3 operator - (vec3 a, vec3 b) {
    return {a.x - b.x, a.y - b.y, a.z - b.z};
}
HD inline vec3 operator - (vec3 a) {
    return {-a.x, -a.y, -a.z};
}
HD inline vec3 operator * (vec3 a, float s) {
    return {a.x * s, a.y * s, a.z * s};
}
HD inline vec3 operator * (float s, vec3 a) {
    return a * s;
}
HD inline vec3 operator / (vec3 a, float s) {
    return a * (1 / s);
}
HD inline float dot(vec3 a, vec3 b) {
    return a.x * b.x + a.y * b.y + a.z * b.z;
}
HD inline vec3 cross(vec3 a, vec3 b) {
    return {a.y * b.z - a.z * b.y, a.z * b.x - a.x * b.z, a.x * b.y - a.y * b.x};
}
HD inline float length(vec3 a) {
    return sqrtf(dot(a, a));
}
HD inline vec3 normalize(vec3 a) {
    float magnitude = length(a);
    return magnitude > 0 ? a / magnitude : vec3{};
}
HD inline vec3 abs(vec3 a) {
    return {fabsf(a.x), fabsf(a.y), fabsf(a.z)};
}
HD inline vec3 max(vec3 a, float b) {
    return {fmaxf(a.x, b), fmaxf(a.y, b), fmaxf(a.z, b)};
}
HD inline vec3 min(vec3 a, float b) {
    return {fminf(a.x, b), fminf(a.y, b), fminf(a.z, b)};
}
#ifndef __CUDACC__
HD inline float min(float a, float b) {
    return fminf(a, b);
}
HD inline float max(float a, float b) {
    return fmaxf(a, b);
}
#endif
HD inline float clamp(float x, float a, float b) {
    return fminf(b, fmaxf(a, x));
}
HD inline vec3 clamp(vec3 x, float a, float b) {
    return min(max(x, a), b);
}
HD inline float mix(float a, float b, float t) {
    return a * (1 - t) + b * t;
}
HD inline vec3 mix(vec3 a, vec3 b, float t) {
    return a * (1 - t) + b * t;
}
