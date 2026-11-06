#include "core/judge_metric.h"
#include "core/cli.h"
#include "core/tasks.h"
int main(int argc,char** argv) { try {
    Cli cli(argc,argv); if(argc<2 || argv[1][0]=='-') throw std::runtime_error("Usage: judge simplified.obj [--ref data/part.obj]");
    auto ref=read_obj(cli.get("--ref","data/part.obj")),out=read_obj(argv[1]); auto v=judge_mesh(ref,out);
    std::cout<<std::setprecision(10)<<"triangles,hausdorff_pct,mean_pct,valid\n"<<out.tris.size()<<','<<100*v.both.maximum/v.diagonal<<','<<100*v.both.mean/v.diagonal<<','<<(v.valid?1:0)<<'\n';
    panel("judge"); row("Samples per direction",50000); row("Seed",12345); row("Chi (ref / output)",std::to_string(stats(ref).chi)+" / "+std::to_string(v.topology.chi)); row("Reference -> output ms",v.forward.ms); row("Output -> reference ms",v.backward.ms); print_verdict(v); print_tasks(); return 0;
} catch(const std::exception& e) { std::cerr<<e.what()<<'\n'; return 1; } }
