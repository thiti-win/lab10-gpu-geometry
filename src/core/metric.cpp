#include "distance.h"
#include "student.h"
#include "timer.h"
Distance one_sided(const Mesh& from,const Mesh& to,int n) {
    auto points=sample_surface(from,n,12345); auto triangles=triangle_vertices(to);
    // Call on the host too, so unfinished tasks are visible in a CUDA build.
#ifdef __CUDACC__
    extern void probe_metric(); probe_metric();
#endif
    vec3* p=gpu_alloc<vec3>(n); vec3* t=gpu_alloc<vec3>(triangles.size()); float* mx=gpu_alloc<float>(1); float* sum=gpu_alloc<float>(1);
    float zero=0; to_gpu(p,points.data(),n); to_gpu(t,triangles.data(),triangles.size()); to_gpu(mx,&zero,1); to_gpu(sum,&zero,1);
    Timer timer; LAUNCH(distance_kernel,dim3((n+255)/256),dim3(256),p,n,t,(int)to.tris.size(),mx,sum); double ms=timer.kernel_ms();
    float a,b; from_gpu(&a,mx,1); from_gpu(&b,sum,1); gpu_free(p); gpu_free(t); gpu_free(mx); gpu_free(sum);
    return {a,b/n,ms};
}
Distance host_distance(const Mesh& from,const Mesh& to,int n,bool parallel) {
    auto p=sample_surface(from,n,12345); auto t=triangle_vertices(to); std::vector<float> ds(n);
    Timer timer; unsigned nt=parallel?std::max(1u,std::thread::hardware_concurrency()):1;
    auto loop=[&](unsigned id) { for(size_t i=id;i<p.size();i+=nt) {
        float d=1e30f; for(size_t j=0;j<t.size();j+=3) d=min(d,point_triangle_distance(p[i],t[j],t[j+1],t[j+2])); ds[i]=d;
    }};
    if(!parallel) loop(0); else { std::vector<std::thread> ts; for(unsigned k=0;k<nt;++k) ts.emplace_back(loop,k); for(auto& t:ts) t.join(); }
    double sum=0,mx=0; for(float d:ds) { sum+=d; mx=std::max(mx,(double)d); } return {mx,sum/n,timer.ms()};
}
