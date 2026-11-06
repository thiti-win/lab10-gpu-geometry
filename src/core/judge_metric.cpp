#include "judge_metric.h"
#include "timer.h"
#include "cli.h"
#include <numeric>
// Independent double-precision projection and segment tests.
struct D3 { double x,y,z; };
HD static D3 dsub(D3 a,D3 b) { return {a.x-b.x,a.y-b.y,a.z-b.z}; }
HD static D3 dadd(D3 a,D3 b) { return {a.x+b.x,a.y+b.y,a.z+b.z}; }
HD static D3 dmul(D3 a,double t) { return {a.x*t,a.y*t,a.z*t}; }
HD static double ddot(D3 a,D3 b) { return a.x*b.x+a.y*b.y+a.z*b.z; }
HD static D3 dcross(D3 a,D3 b) { return {a.y*b.z-a.z*b.y,a.z*b.x-a.x*b.z,a.x*b.y-a.y*b.x}; }
HD static D3 convert(vec3 a) { return {a.x,a.y,a.z}; }
HD static double edge2(D3 p,D3 a,D3 b) {
    D3 e=dsub(b,a); double den=ddot(e,e),t=den?fmin(1.,fmax(0.,ddot(dsub(p,a),e)/den)):0;
    D3 d=dsub(p,dadd(a,dmul(e,t))); return ddot(d,d);
}
HD static double reference_distance2(vec3 pp,vec3 aa,vec3 bb,vec3 cc) {
    D3 p=convert(pp),a=convert(aa),b=convert(bb),c=convert(cc);
    D3 ab=dsub(b,a),ac=dsub(c,a),n=dcross(ab,ac),ap=dsub(p,a); double nn=ddot(n,n);
    if(nn>1e-30) {
        double height=ddot(ap,n),u=ddot(dcross(ap,ac),n)/nn,v=ddot(dcross(ab,ap),n)/nn;
        if(u>=0 && v>=0 && u+v<=1) return height*height/nn;
    }
    return fmin(edge2(p,a,b),fmin(edge2(p,b,c),edge2(p,c,a)));
}
#ifdef __CUDACC__
__global__ void judge_distance_kernel(const vec3* p,int n,const vec3* t,int nt,float* ds) {
    int i=blockIdx.x*blockDim.x+threadIdx.x; if(i>=n) return;
    double best=1e100; for(int j=0;j<nt;++j) best=fmin(best,reference_distance2(p[i],t[3*j],t[3*j+1],t[3*j+2]));
    ds[i]=(float)sqrt(best);
}
#else
// CPU reference uses a BVH to make all eight calibration runs practical.
// It prunes by exact bounding-box lower bounds; leaf distances use the code above.
struct Node { vec3 lo,hi; int begin,end,left=-1,right=-1; };
class ReferenceTree {
    const Mesh& m; std::vector<int> ids; std::vector<Node> nodes;
    int build(int begin,int end) {
        Node n{{1e30f,1e30f,1e30f},{-1e30f,-1e30f,-1e30f},begin,end};
        for(int i=begin;i<end;++i) { auto t=m.tris[ids[i]]; for(int v:{t.x,t.y,t.z}) { auto p=m.positions[v]; n.lo={min(n.lo.x,p.x),min(n.lo.y,p.y),min(n.lo.z,p.z)}; n.hi={max(n.hi.x,p.x),max(n.hi.y,p.y),max(n.hi.z,p.z)}; } }
        int index=(int)nodes.size(); nodes.push_back(n);
        if(end-begin>8) {
            vec3 size=n.hi-n.lo; int axis=size.y>size.x?1:0; if(size.z>(axis?size.y:size.x)) axis=2;
            auto centroid=[&](int i) { auto t=m.tris[i]; vec3 p=m.positions[t.x]+m.positions[t.y]+m.positions[t.z]; return axis==0?p.x:axis==1?p.y:p.z; };
            int mid=(begin+end)/2; std::nth_element(ids.begin()+begin,ids.begin()+mid,ids.begin()+end,[&](int a,int b){return centroid(a)<centroid(b);});
            nodes[index].left=build(begin,mid); nodes[index].right=build(mid,end);
        }
        return index;
    }
    static double lower(vec3 p,const Node& n) {
        double x=fmax(0.,fmax((double)n.lo.x-p.x,(double)p.x-n.hi.x));
        double y=fmax(0.,fmax((double)n.lo.y-p.y,(double)p.y-n.hi.y));
        double z=fmax(0.,fmax((double)n.lo.z-p.z,(double)p.z-n.hi.z)); return x*x+y*y+z*z;
    }
    void search(int id,vec3 p,double& best) const {
        const Node& n=nodes[id]; if(lower(p,n)>best) return;
        if(n.left<0) { for(int i=n.begin;i<n.end;++i) { auto t=m.tris[ids[i]]; best=fmin(best,reference_distance2(p,m.positions[t.x],m.positions[t.y],m.positions[t.z])); } return; }
        int a=n.left,b=n.right; if(lower(p,nodes[a])>lower(p,nodes[b])) std::swap(a,b); search(a,p,best); search(b,p,best);
    }
public:
    explicit ReferenceTree(const Mesh& mesh):m(mesh),ids(mesh.tris.size()) { std::iota(ids.begin(),ids.end(),0); build(0,(int)ids.size()); }
    double distance(vec3 p) const { double best=1e100; search(0,p,best); return sqrt(best); }
};
#endif
Distance judge_one_sided(const Mesh& from,const Mesh& to,int n) {
    auto p=sample_surface(from,n,12345); std::vector<float> ds(n); double ms;
#ifdef __CUDACC__
    auto tris=triangle_vertices(to); auto* dp=gpu_alloc<vec3>(p.size()); auto* dt=gpu_alloc<vec3>(tris.size()); auto* dd=gpu_alloc<float>(n);
    to_gpu(dp,p.data(),p.size()); to_gpu(dt,tris.data(),tris.size()); Timer timer;
    LAUNCH(judge_distance_kernel,dim3((n+255)/256),dim3(256),dp,n,dt,(int)to.tris.size(),dd); ms=timer.kernel_ms();
    from_gpu(ds.data(),dd,n); gpu_free(dp); gpu_free(dt); gpu_free(dd);
#else
    ReferenceTree tree(to); Timer timer; std::vector<std::thread> workers; unsigned nt=std::max(1u,std::thread::hardware_concurrency());
    for(unsigned k=0;k<nt;++k) workers.emplace_back([&,k]{for(int i=k;i<n;i+=nt) ds[i]=(float)tree.distance(p[i]);});
    for(auto& t:workers) t.join(); ms=timer.ms();
#endif
    double mx=0,sum=0; for(float d:ds) { mx=std::max(mx,(double)d); sum+=d; } return {mx,sum/n,ms};
}
Verdict judge_mesh(const Mesh& ref,const Mesh& out) {
    Verdict v; v.topology=stats(out); v.diagonal=bbox_diagonal(ref);
    v.forward=judge_one_sided(ref,out); v.backward=judge_one_sided(out,ref); v.both=two_sided(v.forward,v.backward);
    v.valid=v.topology.manifold && v.topology.components==1 && v.topology.chi==stats(ref).chi && v.both.maximum<=.005*v.diagonal; return v;
}
void print_verdict(const Verdict& v) {
    row("Hausdorff",v.both.maximum); row("Hausdorff %",100*v.both.maximum/v.diagonal); row("Mean distance",v.both.mean); row("Mean %",100*v.both.mean/v.diagonal);
    row("Boundary / nonmanifold",std::to_string(v.topology.boundary)+" / "+std::to_string(v.topology.nonmanifold));
    row("Face components / genus",std::to_string(v.topology.components)+" / "+std::to_string(v.topology.genus));
    row("Competition",v.valid?"VALID":"NOT VALID");
}
