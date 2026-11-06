#pragma once
#include "vec3.h"
#include <vector>
#include <string>
#ifndef __CUDACC__
struct int3 { int x,y,z; };
#endif
struct Mesh { std::vector<vec3> positions; std::vector<int3> tris; };
struct MeshStats { int V=0,E=0,F=0,chi=0,boundary=0,nonmanifold=0,components=0; double genus=-1; bool manifold=true; };
Mesh read_obj(const std::string&);
void write_obj(const Mesh&,const std::string&);
MeshStats stats(const Mesh&);
double bbox_diagonal(const Mesh&);
std::vector<vec3> sample_surface(const Mesh&,int,unsigned);
std::vector<vec3> triangle_vertices(const Mesh&);
