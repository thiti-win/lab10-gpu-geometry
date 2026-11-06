#pragma once
// The same student code runs on CUDA or on standard C++17 CPU threads.
// HD marks functions available on both host and device.
// Given code uses LAUNCH and memory helpers; students write one thread's work.
// CPU launches set thread-local CUDA indices and use mutex-protected atomics.
#include <algorithm>
#include <cstring>
#include <mutex>
#include <stdexcept>
#include <string>
#include <thread>
#include <vector>
#ifdef __CUDACC__
#include <cuda_runtime.h>
#define HD __host__ __device__
inline void cuda_check(cudaError_t e) {
    if (e != cudaSuccess) throw std::runtime_error(cudaGetErrorString(e));
}
#define LAUNCH(kernel, grid, block, ...) do { kernel<<<grid, block>>>(__VA_ARGS__); cuda_check(cudaGetLastError()); cuda_check(cudaDeviceSynchronize()); } while (0)
template<class T> T* gpu_alloc(size_t n) { T* p; cuda_check(cudaMalloc((void**)&p, n*sizeof(T))); return p; }
template<class T> void gpu_free(T* p) { cuda_check(cudaFree(p)); }
template<class T> void to_gpu(T* d, const T* s, size_t n) { cuda_check(cudaMemcpy(d,s,n*sizeof(T),cudaMemcpyHostToDevice)); }
template<class T> void from_gpu(T* d, const T* s, size_t n) { cuda_check(cudaMemcpy(d,s,n*sizeof(T),cudaMemcpyDeviceToHost)); }
#else
#define HD
#define __global__
#define __device__
#define __host__
struct dim3 {
    unsigned x=1,y=1,z=1;
    constexpr dim3(unsigned a=1,unsigned b=1,unsigned c=1):x(a),y(b),z(c) {}
};
inline thread_local dim3 blockIdx, threadIdx, blockDim, gridDim;
template<class Kernel, class... Args>
void launch_cpu(Kernel kernel, dim3 grid, dim3 block, Args... args) {
    unsigned count=grid.x*grid.y*grid.z;
    unsigned nt=std::min(count,std::max(1u,std::thread::hardware_concurrency()));
    std::vector<std::thread> workers;
    for (unsigned t=0;t<nt;++t) workers.emplace_back([=] {
        blockDim=block; gridDim=grid;
        for (unsigned b=t;b<count;b+=nt) {
            blockIdx=dim3(b%grid.x,(b/grid.x)%grid.y,b/(grid.x*grid.y));
            for (unsigned z=0;z<block.z;++z) for (unsigned y=0;y<block.y;++y)
                for (unsigned x=0;x<block.x;++x) { threadIdx=dim3(x,y,z); kernel(args...); }
        }
    });
    for (auto& t:workers) t.join();
}
#define LAUNCH(kernel, grid, block, ...) launch_cpu(kernel,grid,block,__VA_ARGS__)
template<class T> T* gpu_alloc(size_t n) { return new T[n](); }
template<class T> void gpu_free(T* p) { delete[] p; }
template<class T> void to_gpu(T* d,const T* s,size_t n) { std::memcpy(d,s,n*sizeof(T)); }
template<class T> void from_gpu(T* d,const T* s,size_t n) { std::memcpy(d,s,n*sizeof(T)); }
#endif
inline std::mutex atomic_mutex;
HD inline void atomic_add_float(float* p,float v) {
#ifdef __CUDA_ARCH__
    atomicAdd(p,v);
#else
    std::lock_guard<std::mutex> lock(atomic_mutex); *p+=v;
#endif
}
HD inline void atomic_max_float(float* p,float v) {
#ifdef __CUDA_ARCH__
    atomicMax((int*)p,__float_as_int(v));
#else
    std::lock_guard<std::mutex> lock(atomic_mutex); *p=std::max(*p,v);
#endif
}
HD inline void atomic_add_int(int* p,int v) {
#ifdef __CUDA_ARCH__
    atomicAdd(p,v);
#else
    std::lock_guard<std::mutex> lock(atomic_mutex); *p+=v;
#endif
}
inline std::string backend_name() {
#ifdef __CUDACC__
    int d; cuda_check(cudaGetDevice(&d)); cudaDeviceProp p{}; cuda_check(cudaGetDeviceProperties(&p,d));
    return std::string("GPU: ")+p.name+" (sm_"+std::to_string(p.major)+std::to_string(p.minor)+")";
#else
    return "CPU emulation: "+std::to_string(std::max(1u,std::thread::hardware_concurrency()))+" threads";
#endif
}
