#pragma once
#include "vec3.h"

// Given: signed distance to an origin-centred box with these half-sizes.
HD inline float sdf_box(vec3 p, vec3 half_size) {
    vec3 offset = abs(p) - half_size;
    float inside = min(max(offset.x, max(offset.y, offset.z)), 0.0f);
    return length(max(offset, 0)) + inside;
}

// Given: intersection is not needed by the scene in this lab.
HD inline float op_intersect(float a, float b) {
    return max(a, b);
}

// Given: current_scene selects your ball until Task 1d is finished.
HD float current_scene(vec3, float);
// Six nearby samples find the outward direction used by the given lighting.
HD inline vec3 sdf_normal(vec3 p, float k) {
    const float h = 0.001f;
    float dx = current_scene(p + vec3{h, 0, 0}, k) - current_scene(p - vec3{h, 0, 0}, k);
    float dy = current_scene(p + vec3{0, h, 0}, k) - current_scene(p - vec3{0, h, 0}, k);
    float dz = current_scene(p + vec3{0, 0, h}, k) - current_scene(p - vec3{0, 0, h}, k);
    return normalize({dx, dy, dz});
}
