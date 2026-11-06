#include "core/distance.h"
#include "core/cli.h"
#include "core/tasks.h"
int main(int argc,char** argv) { try {
    Cli cli(argc,argv);
    std::string input_a = "data/part.obj";
    std::string input_b = "out/simplified.obj";
    if (argc > 1 && argv[1][0] != '-') {
        if (argc < 3 || argv[2][0] == '-') throw std::runtime_error("Usage: metric A.obj B.obj [--samples N] [--compare]");
        input_a = argv[1];
        input_b = argv[2];
    }
    int n=cli.integer("--samples",20000); positive(n,"Samples"); auto a=read_obj(input_a),b=read_obj(input_b);
    auto ab=one_sided(a,b,n),ba=one_sided(b,a,n),both=two_sided(ab,ba); double diagonal=bbox_diagonal(a);
    panel("metric"); std::cout << std::setprecision(10); row("Backend",backend_name()); row("Input A",input_a); row("Input B",input_b); row("Samples each direction",n); row("A -> B max / mean",std::to_string(ab.maximum)+" / "+std::to_string(ab.mean)); row("B -> A max / mean",std::to_string(ba.maximum)+" / "+std::to_string(ba.mean)); row("A -> B kernel ms",ab.ms); row("B -> A kernel ms",ba.ms); row("A -> B maximum",ab.maximum); row("A -> B mean",ab.mean); row("B -> A maximum",ba.maximum); row("B -> A mean",ba.mean); row("Hausdorff",both.maximum); row("Mean",both.mean); row("Hausdorff %",100*both.maximum/diagonal); row("Mean %",100*both.mean/diagonal);
    if(cli.has("--compare")) {
        auto s1 = host_distance(a,b,n,false), s2 = host_distance(b,a,n,false);
        auto h1 = host_distance(a,b,n,true), h2 = host_distance(b,a,n,true);
        row("A -> B CPU single ms",s1.ms);
        row("B -> A CPU single ms",s2.ms);
        row("A -> B CPU multi ms",h1.ms);
        row("B -> A CPU multi ms",h2.ms);
        row("CPU max difference",fabs(two_sided(h1,h2).maximum-both.maximum));
        row("CPU mean difference",fabs(two_sided(h1,h2).mean-both.mean));
#ifdef __CUDACC__
        row("CPU / GPU speedup",(h1.ms+h2.ms)/(ab.ms+ba.ms));
#else
        row("GPU","not available");
#endif
    }
    print_tasks(); return 0;
} catch(const std::exception& e) { std::cerr<<e.what()<<'\n'; return 1; } }
