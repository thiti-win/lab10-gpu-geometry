#include "core/student.h"
#include "core/quadric.h"
#include "core/distance.h"
#include "core/judge_metric.h"
#include "core/cli.h"
#include <sstream>
bool near(double a,double b,double eps=1e-4) { return fabs(a-b)<eps; }
int main(int argc, char** argv) { try {
    Cli cli(argc, argv);
    int passed=0,total=0;
    auto test=[&](const char* id,bool ok,const std::string& detail,const char* hint) {
        ++total; if(ok) { ++passed; std::cout<<'['<<id<<"] OK\n"; }
        else std::cout<<'['<<id<<"] FAIL: "<<detail<<". Hint: "<<hint<<'\n';
    };
    const int w=37,h=23,n=w*h; Color sentinel={7,8,9}; std::vector<Color> image(n+512,sentinel); auto* dev=gpu_alloc<Color>(image.size());
    to_gpu(dev,image.data(),image.size()); LAUNCH(gradient_kernel,dim3((w+15)/16,(h+15)/16),dim3(16,16),dev,w,h); from_gpu(image.data(),dev,image.size()); gpu_free(dev);
#ifdef __CUDACC__
    extern void probe_gradient(); probe_gradient();
#endif
    bool corners=near(image[0].x,0) && near(image[0].y,0) && near(image[n-1].x,1) && near(image[n-1].y,1) && near(image[n-1].z,.25);
    bool guards=true; for(size_t i=n;i<image.size();++i) guards=guards && image[i].x==7 && image[i].y==8 && image[i].z==9;
    test("0",corners && guards,"37x23 corner or guard mismatch; last red="+std::to_string(image[n-1].x),"Compute x,y from block and thread indices; check both bounds before writing.");
    float sphere=sdf_sphere({3,0,0},1);
    test("1a",near(sphere,2) && near(sdf_sphere({.5f,0,0},1),-.5) && near(sdf_sphere({},1),-1),"sphere expected 2, returned "+std::to_string(sphere),"Distance to a sphere's surface is length(p)-r; keep inside distances negative.");
    set_trace_ball(true);
    int steps=-1,miss_steps=-1;
    float hit=sphere_trace({0,0,5},{0,0,-1},.2f,&steps);
    float miss=sphere_trace({0,3,5},{0,0,1},.2f,&miss_steps);
    test("1b",near(hit,4,.0011) && miss==-1 && steps>0 && miss_steps>0,
         "unit ball ray expected 4, returned "+std::to_string(hit),
         "Sample the given current_scene; advance t by d and write steps on every exit.");
    set_trace_ball(false);
    float smooth=op_smooth_union(.1f,.2f,.3f);
    bool booleans=near(op_union(-1,2),-1) && near(op_subtract(-1,-2),2) && smooth<.1f && near(smooth,.06666667f) && near(op_smooth_union(0,2,.3f),0) && near(op_smooth_union(-1,2,0),-1);
    test("1c",booleans,"union/subtraction/blend mismatch; blend expected .066667, returned "+std::to_string(smooth),"Union uses min(a,b); subtraction uses max(a,-b). Smooth union is given.");
    int floor_steps;
    float floor_hit=sphere_trace({5,3.2f,5},{0,-1,0},.2f,&floor_steps);
    vec3 floor_normal=sdf_normal({5,-.8f,5},.2f);
    float scene[3]={scene_sdf({0,0,0},.2f),scene_sdf({0,1.15f,0},.2f),scene_sdf({5,-.8f,5},.2f)};
    test("1d",near(floor_hit,4,.0011) && floor_steps>0 && length(floor_normal-vec3{0,1,0})<.001f && near(scene[0],.65f) && near(scene[1],-.5f) && near(scene[2],0) && near(scene_sdf({1,1,1},.2f),sqrt(3.)*.2) && near(scene_sdf({.75f,.75f,.75f},.2f),-.05),"scene (origin, top, floor) expected (.65,-.5,0), returned ("+std::to_string(scene[0])+","+std::to_string(scene[1])+","+std::to_string(scene[2])+")","Call the given sdf_box(p,{0.8f,0.8f,0.8f}), cut it with the origin sphere, blend the top sphere, then union the floor. Check the box corner and inside too.");
    // Each candidate must win a case. An unrestricted optimum must not replace them.
    Quadric q=outer(0,0,1,0); vec3 best;
    double cost=collapse_cost(q,{0,0,1},{0,0,2},&best);
    bool candidates=near(cost,1) && length(best-vec3{0,0,1})<1e-6;
    cost=collapse_cost(q,{0,0,2},{0,0,1},&best);
    candidates=candidates && near(cost,1) && length(best-vec3{0,0,1})<1e-6;
    Quadric corner=outer(1,0,0,0)+outer(0,1,0,0)+outer(0,0,1,0);
    cost=collapse_cost(corner,{-1,0,0},{1,0,0},&best);
    candidates=candidates && near(cost,0) && length(best)<1e-6;
    Quadric shifted=outer(1,0,0,-.2)+outer(0,1,0,0)+outer(0,0,1,0);
    cost=collapse_cost(shifted,{0,0,0},{1,0,0},&best);
    candidates=candidates && near(cost,.04) && length(best)<1e-6;
    test("2a",candidates,"endpoint/midpoint selection or returned cost mismatch",
         "Evaluate v1, v2 and (v1+v2)*0.5; write the cheapest to *best and return its evaluated cost.");
    // 259 points force two blocks. Padding catches threads that ignore n_points.
    std::vector<vec3> points(512,{.2f,.2f,100}); for(int i=0;i<259;++i) points[i]={.2f,.2f,0}; points[0]={.2f,.2f,1}; points[1]={.2f,.2f,2}; points[258]={.2f,.2f,3};
    vec3 tris[3]={{0,0,0},{1,0,0},{0,1,0}}; auto* dp=gpu_alloc<vec3>(points.size()); auto* dt=gpu_alloc<vec3>(3); auto* dm=gpu_alloc<float>(1); auto* ds=gpu_alloc<float>(1); float z=0;
    to_gpu(dp,points.data(),points.size()); to_gpu(dt,tris,3); to_gpu(dm,&z,1); to_gpu(ds,&z,1);
    LAUNCH(distance_kernel,dim3(2),dim3(256),dp,259,dt,1,dm,ds); float mx,sum; from_gpu(&mx,dm,1); from_gpu(&sum,ds,1); gpu_free(dp); gpu_free(dt); gpu_free(dm); gpu_free(ds);
    bool nearest=near(mx,3) && near(sum,6);
    std::ostringstream metric_detail; metric_detail<<"259 points max/sum="<<mx<<"/"<<sum;
    // Each triangle is nearest to a different point. Order must not matter.
    vec3 pair_points[2]={{.2f,.2f,1},{.2f,.2f,8}};
    vec3 pair_tris[6]={{0,0,0},{1,0,0},{0,1,0},{0,0,10},{1,0,10},{0,1,10}};
    dp=gpu_alloc<vec3>(2); dt=gpu_alloc<vec3>(6); dm=gpu_alloc<float>(1); ds=gpu_alloc<float>(1);
    to_gpu(dp,pair_points,2);
    for(int order=0;order<2;++order) {
        if(order) for(int k=0;k<3;++k) std::swap(pair_tris[k],pair_tris[k+3]);
        to_gpu(dt,pair_tris,6); to_gpu(dm,&z,1); to_gpu(ds,&z,1);
        LAUNCH(distance_kernel,dim3(1),dim3(256),dp,2,dt,2,dm,ds);
        from_gpu(&mx,dm,1); from_gpu(&sum,ds,1);
        nearest=nearest && near(mx,2) && near(sum,3);
        metric_detail<<"; two triangles "<<(order?"reversed":"forward")<<" max/sum="<<mx<<"/"<<sum<<" (expected 2/3)";
    }
    gpu_free(dp); gpu_free(dt); gpu_free(dm); gpu_free(ds);
#ifdef __CUDACC__
    extern void probe_metric(); probe_metric();
#endif
    test("3a",nearest,metric_detail.str(),"Keep the smallest distance seen for each point as you visit every triangle, then update both atomic totals. Check i<n_points too.");
    std::cout<<passed<<" of "<<total<<" tasks pass\n"; print_tasks(); return passed==total || (cli.has("--ci") && !unfinished.empty()) ? 0 : 1;
} catch(const std::exception& e) { std::cerr<<e.what()<<'\n'; return 2; } }

