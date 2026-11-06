#pragma once
#include "student.h"
struct Pixel { Color color; int steps,hit; };
HD inline Color heat(float t) {
    t=clamp(t,0,1); return t<.5f?mix(Color{.02f,.04f,.25f},Color{1,.85f,.1f},2*t):mix(Color{1,.85f,.1f},Color{.85f,.03f,.02f},2*t-1);
}
HD inline Pixel trace_pixel(int x,int y,int w,int h,float k,int mode,bool flat = false,bool ready = true) {
    vec3 ro={3.2f,2.5f,4.2f},forward=normalize(vec3{0,.2f,0}-ro);
    vec3 right=normalize(cross(forward,{0,1,0})),up=cross(right,forward);
    vec3 rd=normalize(forward+right*((2*(x+.5f)/w-1)*(float)w/h*.52f)+up*((1-2*(y+.5f)/h)*.52f));
    int steps=0; float t=ready?sphere_trace(ro,rd,k,&steps):-1; Color color=mix(Color{.62f,.77f,.95f},Color{.18f,.32f,.55f},clamp(rd.y+.5f,0,1));
    if(t>=0 && flat) color={1,1,1};
    if(t>=0 && !flat) {
        vec3 p=ro+rd*t,n=sdf_normal(p,k),light=normalize(vec3{-3,6,4}-p);
        float visibility=1,travel=.02f;
        for(int i=0;i<48 && travel<8;++i) { float d=current_scene(p+n*.004f+light*travel,k); visibility=min(visibility,12*d/travel); if(d<.001f) break; travel+=max(d,.01f); }
        float shade=.18f+.82f*max(dot(n,light),0.0f)*clamp(visibility,.15f,1);
        Color base=p.y<-.797f?Color{.5f,.53f,.58f}:Color{.85f,.44f,.18f}; color=base*shade;
        color={sqrtf(color.x),sqrtf(color.y),sqrtf(color.z)};
        if(mode==2) color=(n+vec3{1,1,1})*.5f;
    }
    if(mode==1) color=heat(steps/64.0f);
    // Leave room below the bar for the host-drawn 0, 32, 64+ labels.
    int legend_scale = w / 320 > 0 ? w / 320 : 1;
    // Fixed legend: left is few steps (0), right is many (64+).
    if(mode==1 && y>=h-5*legend_scale-24 && y<h-5*legend_scale-8 && x>=12 && x<12+w/4) color=heat((x-12)/(float)max(1.f,w/4.f-1));
    return {color,steps,t>=0?1:0};
}
__global__ inline void render_kernel(Pixel* img,int w,int h,float k,int mode,bool flat = false,bool ready = true) {
    int x=blockIdx.x*blockDim.x+threadIdx.x,y=blockIdx.y*blockDim.y+threadIdx.y;
    if(x<w && y<h) img[y*w+x]=trace_pixel(x,y,w,h,k,mode,flat,ready);
}
