
#include <cstddef>
#include <iostream>
#include <unistd.h>
#include <sys/wait.h>
#include <JANA/JProcessorMapping.h>

int main() {

    JProcessorMapping m;
    std::cout << m << std::endl;

    m.initialize(JProcessorMapping::AffinityStrategy::MemoryBound,
                 JProcessorMapping::LocalityStrategy::CoreLocal);
    std::cout << m << std::endl;

    m.initialize(JProcessorMapping::AffinityStrategy::MemoryBound,
                 JProcessorMapping::LocalityStrategy::NumaDomainLocal);
    std::cout << m << std::endl;

    m.initialize(JProcessorMapping::AffinityStrategy::ComputeBound,
                 JProcessorMapping::LocalityStrategy::CpuLocal);
    std::cout << m << std::endl;
}




