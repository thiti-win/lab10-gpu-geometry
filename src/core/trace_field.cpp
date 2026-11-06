#include "student.h"

static bool host_trace_ball = false;
#ifdef __CUDACC__
__device__ bool device_trace_ball = false;
#endif

void set_trace_ball(bool ball) {
    host_trace_ball = ball;
#ifdef __CUDACC__
    cuda_check(cudaMemcpyToSymbol(device_trace_ball, &ball, sizeof(ball)));
#endif
}

// Given: the same field for ray steps, normals and shadows.
HD float current_scene(vec3 p, float k) {
#ifdef __CUDA_ARCH__
    bool ball = device_trace_ball;
#else
    bool ball = host_trace_ball;
#endif
    return ball ? sdf_sphere(p, 1.0f) : scene_sdf(p, k);
}

