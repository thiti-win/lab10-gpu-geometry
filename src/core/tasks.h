#pragma once
#include "gpu.h"
#include <iostream>
#include <set>
#include <type_traits>
inline std::set<std::string> unfinished;
inline std::mutex task_mutex;
HD inline void task_not_done(const char* id) {
#ifndef __CUDA_ARCH__
    std::lock_guard<std::mutex> lock(task_mutex); unfinished.insert(id);
#endif
}
inline void print_tasks() { for (auto& id:unfinished) std::cout<<"TASK "<<id<<" not done\n"; }

// Given placeholder: replace each task_blank<T>("id") with your expression.
// It reports the task unfinished on the host; CUDA host probes do the same.
// T is the required result type. Blanks return zero, true for conditions, or an out-of-range integer index.
template<class T> HD inline T task_blank(const char* id) {
#ifndef __CUDA_ARCH__
    // A small per-thread mask avoids repeated locks and needs no TLS destructor.
    unsigned index = id[0] == '0' ? 0 : id[0] == '1' ? 1 + id[1] - 'a'
                     : id[0] == '2' ? 6 + id[1] - 'a' : 8;
    thread_local unsigned reported = 0;
    unsigned bit = 1u << index;
    if (!(reported & bit)) {
        task_not_done(id);
        reported |= bit;
    }
#endif
    if constexpr (std::is_same<T, int>::value) return 0x7fffffff;
    else if constexpr (std::is_same<T, bool>::value) return true;
    else return T{};
}
