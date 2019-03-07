
#include <unistd.h>

#ifdef __APPLE__
#include <mach/thread_policy.h>
#import <mach/thread_act.h>
#include <cpuid.h>
#else //__APPLE__
#include <sched.h>
#endif //__APPLE__

#ifdef HAVE_NUMA
#include <numa.h>
#endif //HAVE_NUMA

#include <JANA/JCpuInfo.h>

namespace JCpuInfo {

size_t GetNumCpus() {
	/// Returns the number of cores that are on the computer.
	/// The count will be full cores+hyperthreads (or equivalent)
	return sysconf(_SC_NPROCESSORS_ONLN);
}


size_t GetCpuID() {
	/// Returns the current CPU the calling thread is running on.
	/// Note that unless the thread affinity has been set, this may 
	/// change, even before returning from this call. The thread
	/// affinity of all threads may be fixed by setting the AFFINITY
	/// configuration parameter at program start up.


#ifdef __APPLE__
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
	// TODO: Clean this up	

#else //__APPLE__
	return sched_getcpu();
#endif //__APPLE__
}



size_t GetNumaNodeID() {
#ifdef HAVE_NUMA
	if (numa_available() == -1) {
		return 0;
	} else {
		return numa_node_of_cpu(GetCpuID());
	}
#else //HAVE_NUMA
	return 0;
#endif //HAVE_NUMA
}

size_t GetNumNumaNodes() {
#ifdef HAVE_NUMA
	if (numa_available() == -1) {
		return 1;
	} else {
		return numa_num_configured_nodes();
	}
#else //HAVE_NUMA
	return 1;
#endif //HAVE_NUMA
}

} // JCpuInfo namespace

