#include "core/trace.h"
#include "core/cli.h"
#include "core/timer.h"
#include "core/image.h"
#include "core/legend.h"
#include <random>
#include <sstream>
void trace_probe() {
    std::cout << "[1a] sphere (3,0,0) -> " << sdf_sphere({3,0,0},1) << " (expected 2)\n";
    std::cout << "[1a] sphere (0.5,0,0) -> " << sdf_sphere({0.5f,0,0},1) << " (expected -0.5)\n";
    std::cout << "given sdf_box (2,0,0) -> " << sdf_box({2,0,0},{1,1,1}) << " (expected 1)\n";
    std::cout << "given sdf_box (2,2,0) -> " << sdf_box({2,2,0},{1,1,1}) << " (expected sqrt(2))\n";
    std::cout << "given sdf_box (0.5,0,0) -> " << sdf_box({0.5f,0,0},{1,1,1}) << " (expected -0.5)\n";
    std::cout << "hand example (unit sphere only, uses your sdf_sphere)\n";
    float t = 0;
    for (int i = 1; i <= MAX_STEPS; ++i) {
        vec3 p = {0,0,5-t};
        float distance = sdf_sphere(p,1);
        std::cout << "step " << i << " p=(0,0," << p.z << ") f=" << distance << '\n';
        if (distance < HIT_EPS) {
            std::cout << "hit at t=" << t << '\n';
            break;
        }
        t += distance;
        if (t > MAX_DIST) break;
    }
    int steps;
    float floor_hit = sphere_trace({5,3.2f,5},{0,-1,0},0.2f,&steps);
    std::cout << "[1b] scene floor ray: t = " << floor_hit << " (expected t = 4), steps = " << steps << '\n';
}
int main(int argc,char** argv) { try {
    Cli cli(argc,argv); panel("trace"); if(cli.has("--probe")) { trace_probe(); print_tasks(); return 0; }
    int w=960,h=540; auto res=cli.get("--res","960x540"); char sep; std::istringstream s(res); if(!(s>>w>>sep>>h) || sep!='x') throw std::runtime_error("Use --res WxH"); positive(w,"Width"); positive(h,"Height");
    float k=std::stof(cli.get("--k","0.2")); auto mode=cli.get("--mode","shade"); if(mode!="shade" && mode!="steps" && mode!="normal") throw std::runtime_error("Unknown mode"); int m=mode=="steps"?1:mode=="normal"?2:0;
    // Exercise every task on the host before the device launch.
    sdf_sphere({},1); sdf_box({},{1,1,1}); op_union(0,1); op_intersect(0,1); op_subtract(0,1); op_smooth_union(0,1,k); scene_sdf({},k); int probe; sphere_trace({0,0,5},{0,0,-1},k,&probe); sdf_normal({},k);
    bool ball = unfinished.count("1d") != 0;
    bool flat = false; // Surface normal and lighting are given.
    bool ready = !unfinished.count("1a") && !unfinished.count("1b");
    // TA-only switches reproduce the intermediate reference pictures.
    if (cli.has("--force-ball")) ball = true;
    if (cli.has("--force-flat")) flat = true;
    set_trace_ball(ball);
    row("drawing", !ready ? "sky (finish 1a and 1b first)" : std::string(ball ? "one ball (1d not done yet)" : "full scene") +
        (flat ? ", flat white (TA preview)" : ", lit with normals"));
    std::vector<Pixel> host(size_t(w)*h); auto* dev=gpu_alloc<Pixel>(host.size()); dim3 block(16,16),grid((w+15)/16,(h+15)/16);
    Timer timer; LAUNCH(render_kernel,grid,block,dev,w,h,k,m,flat,ready); double ms=timer.kernel_ms(); from_gpu(host.data(),dev,host.size()); gpu_free(dev);
    std::vector<Color> image(host.size()); double sum=0,hits=0; int mx=0;
    for(size_t i=0;i<host.size();++i) { image[i]=host[i].color; sum+=host[i].steps; mx=std::max(mx,host[i].steps); hits+=host[i].hit; }
    if (m == 1 && w >= 160 && h >= 60) {
        int scale = std::max(1, w / 320);
        image_number(image,w,h,12,h-5*scale-3,"0");
        image_number(image,w,h,12+w/8-4*scale,h-5*scale-3,"32");
        image_number(image,w,h,12+w/4-12*scale,h-5*scale-3,"64+");
    }
    auto out=cli.get("--out","out/trace.png"); save_png(out,image,w,h);
    row("Resolution",res); row("Backend",backend_name()); row("Block / grid","16 x 16 / "+std::to_string(grid.x)+" x "+std::to_string(grid.y)); row("Render ms",ms); row("Mean steps",sum/host.size()); row("Max steps",mx); row("Hit %",100*hits/host.size());
    if(m==1) row("steps colour scale", "0 (dark) .. 32 (yellow) .. 64+ (red)");
    std::mt19937 rng(12345); auto random=[&] { return (float)((rng()+.5)/4294967296.0*4-2); };
    double lo=1e30,hi=0,total=0; int bad=0; const float e=.001f;
    for(int i=0;i<100000;++i) {
        vec3 p={random(),random(),random()};
        vec3 g={current_scene(p+vec3{e,0,0},k)-current_scene(p-vec3{e,0,0},k),current_scene(p+vec3{0,e,0},k)-current_scene(p-vec3{0,e,0},k),current_scene(p+vec3{0,0,e},k)-current_scene(p-vec3{0,0,e},k)};
        double v=length(g)/(2*e); lo=std::min(lo,v); hi=std::max(hi,v); total+=v; if(v<.99 || v>1.01) ++bad;
    }
    row("Eikonal samples",100000); row("|grad f| min",lo); row("|grad f| mean",total/100000); row("|grad f| max",hi); row("Outside [.99,1.01] %",bad/1000.0);
    if(cli.has("--compare")) {
        double times[2]; std::vector<Pixel> cpu(host.size());
        for(int pass=0;pass<2;++pass) {
            Timer ct; unsigned nt=pass?std::max(1u,std::thread::hardware_concurrency()):1;
            auto loop=[&](unsigned id) { for(int y=id;y<h;y+=nt) for(int x=0;x<w;++x) cpu[y*w+x]=trace_pixel(x,y,w,h,k,m,flat,ready); };
            if(!pass) loop(0); else { std::vector<std::thread> ts; for(unsigned id=0;id<nt;++id) ts.emplace_back(loop,id); for(auto& t:ts) t.join(); }
            times[pass]=ct.ms(); double error=0; for(size_t i=0;i<cpu.size();++i) error=std::max(error,(double)length(cpu[i].color-host[i].color));
            row(pass?"CPU multi ms":"CPU single ms",times[pass]); row("Max color difference",error);
        }
        row("CPU single / multi",times[0]/times[1]);
#ifdef __CUDACC__
        row("CPU single / GPU",times[0]/ms);
#else
        row("GPU","not available");
#endif
    }
    std::string reference;
    if (w == 960 && h == 540 && unfinished.count("1a") == 0 && unfinished.count("1b") == 0) {
        if (ball || flat) {
            if (ball && m == 0) reference = flat ? "reference/steps/white_ball.png" : "reference/steps/1b.png";
        } else {
        if (k == 0.2f) {
            reference = m == 1 ? "reference/trace_steps.png" :
                        m == 2 ? "reference/trace_normal.png" : "reference/trace.png";
        } else if (m == 0 && k == 0) reference = "reference/trace_k0.png";
        else if (m == 0 && k == 1) reference = "reference/trace_k1.png";
        }
    }
    row("Saved",out);
    row("Comparison", save_comparison("trace", reference, image, w, h));
    print_tasks(); return 0;
} catch(const std::exception& e) { std::cerr<<e.what()<<'\n'; return 1; } }
