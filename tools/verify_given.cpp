#include "../src/core/judge_metric.h"
#include "../src/core/distance.h"
#include "../src/core/tasks.h"
#include "../src/core/student.h"
#include "../src/core/quadric.h"
#include <iostream>
#include <random>
#include <fstream>
static void require(bool ok,const char* text){if(!ok)throw std::runtime_error(text);std::cout<<"OK: "<<text<<'\n';}
__global__ void atomic_test(int* sum){atomic_add_int(sum,1);}
int main()try{
  if (fabs(sdf_sphere({3,0,0},1)-2)<1e-6) {
    set_trace_ball(true);
    require(length(sdf_normal({0,1,0},.2f)-vec3{0,1,0})<1e-5,"given ball normal points outward");
    set_trace_ball(false);
  }
  auto horizontal=triangle_plane({0,0,2},{2,0,2},{0,3,2});
  auto tilted=triangle_plane({.2f,.7f,-.4f},{1.3f,-.2f,.8f},{-.3f,.5f,1.2f});
  require(length(horizontal.normal-vec3{0,0,1})<1e-6 && fabs(horizontal.d+2)<1e-6,
          "given triangle plane has unit normal and offset");
  require(fabs(length(tilted.normal)-1)<1e-6 && fabs(dot(tilted.normal,{.2f,.7f,-.4f})+tilted.d)<1e-6,
          "given tilted plane contains its corner");
  auto degenerate=triangle_plane({},{},{1,0,0});
  require(length(degenerate.normal)==0 && degenerate.d==0,"given degenerate plane is zero");

  double matrix[3][3]={{2,1,-1},{-3,-1,2},{-2,1,2}};
  double rhs[3]={8,-11,-3}, solution[3];
  require(solve3x3(matrix,rhs,solution) && fabs(solution[0]-2)<1e-10 && fabs(solution[1]-3)<1e-10 && fabs(solution[2]+1)<1e-10,"Cramer solve with nonsymmetric matrix");
  double singular[3][3]{};
  require(!solve3x3(singular,rhs,solution),"singular solve rejected");
  auto plane=outer(1,2,3,4);
  double block[3][3], column[3];
  quadric_parts(plane,block,column);
  require(block[0][2]==3 && block[2][1]==6 && column[0]==4 && column[2]==12 && evaluate(plane,{1,1,1})==100,"outer product, block extraction and evaluation");
  Mesh tetra;tetra.positions={{0,0,0},{1,0,0},{0,1,0},{0,0,1}};tetra.tris={{0,2,1},{0,1,3},{0,3,2},{1,2,3}};
  write_obj(tetra,"out/helper-roundtrip.obj");
  { std::ofstream out("out/helper-roundtrip.obj",std::ios::app); out<<"\n  \n# Blank lines are legal.\n"; }
  auto loaded=read_obj("out/helper-roundtrip.obj");require(loaded.positions.size()==4&&loaded.tris.size()==4,"OBJ round trip with blank lines");
  auto st=stats(tetra);require(st.V==4&&st.E==6&&st.F==4&&st.chi==2&&st.boundary==0&&st.nonmanifold==0&&st.manifold,"tetrahedron topology");
  require(st.components==1&&st.genus==0,"connected closed surface genus");
  auto disconnected=tetra;
  for(auto p:tetra.positions)disconnected.positions.push_back(p+vec3{3,0,0});
  for(auto t:tetra.tris)disconnected.tris.push_back({t.x+4,t.y+4,t.z+4});
  auto disconnected_stats=stats(disconnected);
  require(disconnected_stats.manifold&&disconnected_stats.components==2&&disconnected_stats.genus==-1,"disconnected closed surfaces have no single genus");
  auto open=tetra;open.tris.pop_back();require(stats(open).boundary==3&&stats(open).genus==-1,"open surface rejected");
  auto duplicate=tetra;duplicate.tris.push_back(tetra.tris[0]);require(stats(duplicate).nonmanifold==3&&!stats(duplicate).manifold,"duplicate face rejected");
  auto pinch=tetra;for(int i=1;i<4;++i)pinch.positions.push_back(-tetra.positions[i]);for(auto t:tetra.tris){auto map=[](int i){return i==0?0:i+3;};pinch.tris.push_back({map(t.x),map(t.y),map(t.z)});}require(stats(pinch).boundary==0&&stats(pinch).nonmanifold==0&&!stats(pinch).manifold,"pinched vertex link rejected");
  require(stats(pinch).components==2&&stats(pinch).genus==-1,"vertex contact does not connect faces");
  auto samples=sample_surface(tetra,997,12345),again=sample_surface(tetra,997,12345);bool repeat=true;for(size_t i=0;i<samples.size();++i)repeat=repeat&&length(samples[i]-again[i])==0;require(repeat,"fixed seed sampling repeats");
  require(fabs(point_triangle_distance({2,0,0},{0,0,0},{1,0,0},{0,1,0})-1)<1e-6,"triangle vertex region");
  require(fabs(point_triangle_distance({.5f,-2,0},{0,0,0},{1,0,0},{0,1,0})-2)<1e-6,"triangle edge region");
  require(fabs(point_triangle_distance({0,0,3},{},{},{})-3)<1e-6,"degenerate triangle distance");
  std::mt19937 rng(9);auto r=[&](){return float(int(rng()%1000)-500)/200;};Mesh a,b;
  for(int i=0;i<41;++i){int n=int(a.positions.size());for(int j=0;j<3;++j)a.positions.push_back({r(),r(),r()});a.tris.push_back({n,n+1,n+2});}b=a;for(auto& p:b.positions)p=p+vec3{.2f,-.1f,.3f};
  auto actual=two_sided(host_distance(a,b,997,true),host_distance(b,a,997,true)),reference=two_sided(judge_one_sided(a,b,997),judge_one_sided(b,a,997));require(fabs(actual.maximum-reference.maximum)<1e-5&&fabs(actual.mean-reference.mean)<1e-5,"BVH judge agrees with independent brute force on random surfaces");
  auto same=judge_mesh(tetra,tetra);require(same.both.maximum<1e-6&&same.valid,"identical closed surface is VALID");
  auto shifted=tetra;for(auto& p:shifted.positions)p=p+vec3{10,0,0};auto far=judge_mesh(tetra,shifted);require(!far.valid,"distant closed surface rejected");
  int zero=0,sum=0;auto p=gpu_alloc<int>(1);to_gpu(p,&zero,1);LAUNCH(atomic_test,dim3(13),dim3(37),p);from_gpu(&sum,p,1);gpu_free(p);require(sum==481,"integer atomics with multiple blocks");
  std::cout<<"All given-code checks passed\n";return 0;
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
