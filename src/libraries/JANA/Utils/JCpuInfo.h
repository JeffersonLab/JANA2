
// Copyright 2020, Jefferson Science Associates, LLC.
// Subject to the terms in the LICENSE file found in the top-level directory.
#pragma once

#ifndef JANA2_CACHE_LINE_BYTES
#define JANA2_CACHE_LINE_BYTES 64
#endif
// The cache line size is 64 for ifarm1402. gcc won't allow larger than 128
// You can find the cache line size in /sys/devices/system/cpu/cpu0/cache/index0/coherency_line_size
/*! The cache line size is useful for creating a buffer to make sure that a variable accessed by multiple threads does not share the cache line it is on.
    This is useful for variables that may be written-to by one of the threads, because the thread will acquire locked access to the entire cache line.
    This blocks other threads from operating on the other data stored on the cache line. Note that it is also important to align the shared data as well.
    See http://www.drdobbs.com/parallel/eliminate-false-sharing/217500206?pgno=4 for more details. */


#include <thread>

namespace JCpuInfo {

    size_t GetNumCpus();

    uint32_t GetCpuID();

    bool PinThreadToCpu(std::thread *thread, size_t cpu_id);

}
