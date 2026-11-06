#include "core/simplify.h"
#include "core/judge_metric.h"
#include "core/tasks.h"
#include "core/cli.h"
#include "core/timer.h"
void mesh_row(const std::string& label,const MeshStats& s) {
    row(label+" V/E/F",std::to_string(s.V)+" / "+std::to_string(s.E)+" / "+std::to_string(s.F));
    row(label+" chi / genus",std::to_string(s.chi)+" / "+std::to_string(s.genus));
}
int main(int argc,char** argv) { try {
    Cli cli(argc,argv); panel("simplify");
    if(cli.has("--probe")) {
        Plane plane=triangle_plane({0,0,0},{1,0,0},{0,1,0});
        row("[given] plane normal",std::to_string(plane.normal.x)+","+std::to_string(plane.normal.y)+","+std::to_string(plane.normal.z));
        row("[given] plane d",plane.d);
        auto q=plane_quadric({0,0,0},{1,0,0},{0,1,0});
        std::cout<<"evaluate(Q,(0,0,2)) = "<<evaluate(q,{0,0,2})<<" (expected 4)\n";
        Plane shifted=triangle_plane({0,0,2},{2,0,2},{0,3,2});
        row("[given] z=2 plane d",shifted.d);
        Quadric corner = outer(1,0,0,0) + outer(0,1,0,0) + outer(0,0,1,0);
        vec3 p;
        double cost=collapse_cost(corner,{-1,0,0},{1,0,0},&p);
        row("[2a] Corner best",std::to_string(p.x)+","+std::to_string(p.y)+","+std::to_string(p.z));
        row("[2a] Corner cost",cost);
        print_tasks(); return 0;
    }
    int target=cli.integer("--target",1000); positive(target,"Target"); auto input=read_obj(cli.get("--in","data/part.obj"));
    if (cli.has("--unsafe")) row("safety", "link check OFF (--unsafe)");
    CollapseStats counts; Timer timer; auto result=simplify_mesh(input,target,counts,cli.has("--unsafe")); double ms=timer.ms();
    auto out=cli.get("--out","out/simplified.obj"); write_obj(result,out);
    mesh_row("Input",stats(input)); mesh_row("Output",stats(result)); row("Collapses",counts.done); row("Rejected link",counts.link); row("Rejected normal flip",counts.flip); row("Rejected degenerate",counts.degenerate); row("Simplify ms",ms); row("Saved",out);
    // The assignment metric is displayed, then the independent competition verdict.
    auto a=one_sided(input,result,20000),b=one_sided(result,input,20000); auto d=two_sided(a,b); double diag=bbox_diagonal(input);
    if (unfinished.count("3a")) {
        row("Part III error", "(Part III not done yet)");
    } else {
    row("Part III Hausdorff",d.maximum); row("Part III Hausdorff %",100*d.maximum/diag); row("Part III mean",d.mean); row("Part III mean %",100*d.mean/diag);
    }
    auto v=judge_mesh(input,result); print_verdict(v); print_tasks(); return 0;
} catch(const std::exception& e) { std::cerr<<e.what()<<'\n'; return 1; } }
