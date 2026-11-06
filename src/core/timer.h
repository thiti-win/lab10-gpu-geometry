#pragma once
#include "gpu.h"
#include <chrono>
class Timer {
    std::chrono::steady_clock::time_point start=std::chrono::steady_clock::now();
#ifdef __CUDACC__
    cudaEvent_t a{},b{};
#endif
public:
    Timer() {
#ifdef __CUDACC__
        cuda_check(cudaEventCreate(&a)); cuda_check(cudaEventCreate(&b)); cuda_check(cudaEventRecord(a));
#endif
    }
    ~Timer() {
#ifdef __CUDACC__
        cudaEventDestroy(a); cudaEventDestroy(b);
#endif
    }
    double ms() const { return std::chrono::duration<double,std::milli>(std::chrono::steady_clock::now()-start).count(); }
    double kernel_ms() {
#ifdef __CUDACC__
        cuda_check(cudaEventRecord(b)); cuda_check(cudaEventSynchronize(b)); float t; cuda_check(cudaEventElapsedTime(&t,a,b)); return t;
#else
        return ms();
#endif
    }
};
