// Compiled as ordinary C++ even in the CUDA Makefile.
// A namespace gives the same student kernels distinct host symbols.
#include "core/student.h"
#include "core/distance.h"
namespace host_copy {
#include "../student/part0_hello.cu"
#include "../student/part3_metric.cu"
}
void probe_gradient() {
    blockIdx=dim3(0,0,0); threadIdx=dim3(0,0,0); blockDim=dim3(1); gridDim=dim3(1);
    Color c[4]{}; host_copy::gradient_kernel(c,2,2);
}
void probe_metric() {
    blockIdx=dim3(0,0,0); threadIdx=dim3(0,0,0); blockDim=dim3(1); gridDim=dim3(1);
    vec3 p={0,0,1},t[3]={{0,0,0},{1,0,0},{0,1,0}}; float a=0,b=0; host_copy::distance_kernel(&p,1,t,1,&a,&b);
}
