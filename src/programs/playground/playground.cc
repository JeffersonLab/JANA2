
using namespace std;
#include <cstddef>
#include <iostream>

#import <JANA/JCpuInfo.h>
int main() {
    size_t ncpus = JCpuInfo::GetNumCpus();
    for (size_t i=0; i<ncpus; ++i) {
        std::cout << i << ": " << JCpuInfo::GetNumaNodeID(i) << std::endl;
    }
}



