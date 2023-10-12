
// Copyright 2020, Jefferson Science Associates, LLC.
// Subject to the terms in the LICENSE file found in the top-level directory.
#pragma once

#ifndef JANA2_CACHE_LINE_BYTES
#define JANA2_CACHE_LINE_BYTES 64
#endif

#include <thread>

namespace JCpuInfo {

    size_t GetNumCpus();

    uint32_t GetCpuID();

    size_t GetNumaNodeID();

    size_t GetNumaNodeID(size_t cpuid);

    size_t GetNumNumaNodes();

    bool PinThreadToCpu(std::thread *thread, size_t cpu_id);

}