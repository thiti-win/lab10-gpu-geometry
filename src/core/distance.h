#pragma once
#include "mesh.h"
// Ericson's closest point regions. Degenerate triangles reduce to segments.
HD inline float segment_distance(vec3 p,vec3 a,vec3 b) {
    vec3 e=b-a; float d=dot(e,e); return length(p-(a+e*(d>0?clamp(dot(p-a,e)/d,0,1):0)));
}
// Given point p and triangle corners a,b,c, return the nearest distance.
// This helper works on CPU and GPU; degenerate triangles become segments.
HD inline float point_triangle_distance(vec3 p,vec3 a,vec3 b,vec3 c) {
    vec3 ab=b-a,ac=c-a,ap=p-a;
    if(dot(cross(ab,ac),cross(ab,ac))<1e-24f) return min(segment_distance(p,a,b),min(segment_distance(p,b,c),segment_distance(p,c,a)));
    float d1=dot(ab,ap),d2=dot(ac,ap);
    if(d1<=0 && d2<=0) return length(ap);
    vec3 bp=p-b; float d3=dot(ab,bp),d4=dot(ac,bp);
    if(d3>=0 && d4<=d3) return length(bp);
    float vc=d1*d4-d3*d2;
    if(vc<=0 && d1>=0 && d3<=0) return length(p-(a+ab*(d1/(d1-d3))));
    vec3 cp=p-c; float d5=dot(ab,cp),d6=dot(ac,cp);
    if(d6>=0 && d5<=d6) return length(cp);
    float vb=d5*d2-d1*d6;
    if(vb<=0 && d2>=0 && d6<=0) return length(p-(a+ac*(d2/(d2-d6))));
    float va=d3*d6-d5*d4;
    if(va<=0 && d4-d3>=0 && d5-d6>=0) return length(p-(b+(c-b)*((d4-d3)/((d4-d3)+(d5-d6)))));
    float inv=1/(va+vb+vc); return length(p-(a+ab*(vb*inv)+ac*(vc*inv)));
}
struct Distance { double maximum=0,mean=0,ms=0; };
Distance one_sided(const Mesh&,const Mesh&,int);
Distance host_distance(const Mesh&,const Mesh&,int,bool);
inline Distance two_sided(Distance a,Distance b) { return {std::max(a.maximum,b.maximum),(a.mean+b.mean)/2,a.ms+b.ms}; }
inline Distance two_sided(const Mesh& a,const Mesh& b,int samples) {
    auto ab=one_sided(a,b,samples),ba=one_sided(b,a,samples); return two_sided(ab,ba);
}
