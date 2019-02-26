#include <unistd.h>

#include <JANA/JCpuInfo.h>

namespace JCpuInfo {

size_t GetNumCpus() {
	/// Returns the number of cores that are on the computer.
	/// The count will be full cores+hyperthreads (or equivalent)
	return sysconf(_SC_NPROCESSORS_ONLN);
}


#ifdef __APPLE__
#include <mach/thread_policy.h>
#import <mach/thread_act.h>
#include <cpuid.h>

size_t GetCpuID() {
	/// Returns the current CPU the calling thread is running on.
	/// Note that unless the thread affinity has been set, this may 
	/// change, even before returning from this call. The thread
	/// affinity of all threads may be fixed by setting the AFFINITY
	/// configuration parameter at program start up.

	// TODO: Clean this up	
// From https://stackoverflow.com/questions/33745364/sched-getcpu-equivalent-for-os-x
#define CPUID(INFO, LEAF, SUBLEAF) __cpuid_count(LEAF, SUBLEAF, INFO[0], INFO[1], INFO[2], INFO[3])
#define GETCPU(CPU) {                              \
        uint32_t CPUInfo[4];                           \
        CPUID(CPUInfo, 1, 0);                          \
        /* CPUInfo[1] is EBX, bits 24-31 are APIC ID */ \
        if ( (CPUInfo[3] & (1 << 9)) == 0) {           \
          CPU = -1;  /* no APIC on chip */             \
        }                                              \
        else {                                         \
          CPU = (unsigned)CPUInfo[1] >> 24;                    \
        }                                              \
        if (CPU < 0) CPU = 0;                          \
        }

	int cpuid;
	GETCPU(cpuid);
	return cpuid;
}

// macOS does not support NUMA yet
size_t GetNumaNodeID() {
	return 0;
}

size_t GetNumNumaNodes() {
	return 1;
}


#else

#include <numa.h>
#include <sched.h>

size_t GetCpuID() {
	return sched_getcpu();
}

size_t GetNumaNodeID() {
	if (numa_available() == -1) {
		return 0;
	} else {
		return numa_node_of_cpu(GetCpuID());
	}
}


size_t GetNumNumaNodes() {
	if (numa_available() == -1) {
		return 1;
	} else {
		return numa_num_configured_nodes();
	}
}



#endif // __APPLE__
}

