#include "core/student.h"
#include "core/timer.h"
#include "core/cli.h"
#include "core/image.h"
int main(int argc,char** argv) { try {
    Cli cli(argc,argv); int w=1024,h=768; dim3 block(16,16),grid((w+15)/16,(h+15)/16);
#ifdef __CUDACC__
    extern void probe_gradient(); probe_gradient();
#endif
    Color* img=gpu_alloc<Color>(w*h); std::vector<Color> host(w*h);
    // Initialize for a safe black image in the student version.
    to_gpu(img,host.data(),host.size()); Timer timer; LAUNCH(gradient_kernel,grid,block,img,w,h); double ms=timer.kernel_ms();
    from_gpu(host.data(),img,host.size()); gpu_free(img); auto out=cli.get("--out","out/hello.png"); save_png(out,host,w,h);
    panel("hello"); row("Backend",backend_name()); row("Resolution","1024 x 768"); row("Block / grid","16 x 16 / 64 x 48"); row("Threads / pixels",std::to_string(grid.x*grid.y*256)+" / "+std::to_string(w*h)); row("Render ms",ms); row("Saved",out);
    row("Comparison", save_comparison("hello", "reference/hello.png", host, w, h));
    print_tasks(); return 0;
} catch(const std::exception& e) { std::cerr<<e.what()<<'\n'; return 1; } }
