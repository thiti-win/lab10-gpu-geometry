#pragma once
#include "vec3.h"
#include "sdf_given.h"
#include "tasks.h"
constexpr int MAX_STEPS=256;
constexpr float HIT_EPS=1e-3f, MAX_DIST=20;
__global__ void gradient_kernel(Color*,int,int);
HD float sdf_sphere(vec3,float);
HD float op_union(float,float);
HD float op_subtract(float,float);
HD float op_smooth_union(float,float,float);
HD float scene_sdf(vec3,float);
// current_scene(p,k) draws your ball before 1d, your scene after.
void set_trace_ball(bool);
HD float current_scene(vec3,float);
HD float sphere_trace(vec3,vec3,float,int*);
__global__ void distance_kernel(const vec3*,int,const vec3*,int,float*,float*);
