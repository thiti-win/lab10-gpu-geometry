#pragma once
#include "distance.h"
// Independent reference. Never calls distance_kernel or any student function.
Distance judge_one_sided(const Mesh&,const Mesh&,int n=50000);
struct Verdict { MeshStats topology; Distance forward,backward,both; double diagonal=0; bool valid=false; };
Verdict judge_mesh(const Mesh& reference,const Mesh& output);
void print_verdict(const Verdict&);
