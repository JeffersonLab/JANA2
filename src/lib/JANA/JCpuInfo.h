#pragma once

#include <thread>

namespace JCpuInfo {

    size_t GetNumCpus();

    size_t GetCpuID();

    size_t GetNumaNodeID();

    size_t GetNumaNodeID(size_t cpuid);

    size_t GetNumNumaNodes();

    bool PinThreadToCpu(std::thread *thread, size_t cpu_id);

}