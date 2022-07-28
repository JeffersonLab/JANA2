
// Copyright 2020, Jefferson Science Associates, LLC.
// Subject to the terms in the LICENSE file found in the top-level directory.

#include <unistd.h>
#include <thread>
#include <typeinfo>

// Note that Apple complicates things some. In particular with the
// addition of Apple silicon (M1 chip) which does not seem to have
// thesame CPU utilities as older Apple OS versions/hardware (ugh!)
#ifdef __APPLE__
#include <mach/thread_policy.h>
#import <mach/thread_act.h>
#ifndef __aarch64__
#include <cpuid.h>
#endif // __aarch64__
#else //__APPLE__ (i.e. Linux)
#include <sched.h>
#endif //__APPLE__

#ifdef HAVE_NUMA
#include <numa.h>
#endif //HAVE_NUMA

#include <JANA/Utils/JCpuInfo.h>

extern thread_local int THREAD_ID;


namespace JCpuInfo {

size_t GetNumCpus() {
    /// Returns the number of cores that are on the computer.
    /// The count will be full cores+hyperthreads (or equivalent)
    return sysconf(_SC_NPROCESSORS_ONLN);
}


uint32_t GetCpuID() {
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

    int cpuid=0;
// Apple M1 running MacOS 12.0.1 does not support __cpuid_count. Skip for now
#ifdef __cpuid_count
    GETCPU(cpuid);
#else  // __cpuid_count
#warning __cpuid_count is not defined on this system.
#endif // __cpuid_count
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

size_t GetNumaNodeID(size_t cpu_id) {
#ifdef HAVE_NUMA
        if (numa_available() == -1) {
        return 0;
    } else {
        return numa_node_of_cpu(cpu_id);
    }
#else //HAVE_NUMA
	cpu_id = 0; // suppress compiler warning.
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


bool PinThreadToCpu(std::thread* thread, size_t cpu_id) {

    if (typeid(std::thread::native_handle_type) != typeid(pthread_t)) {
        return false;
    }
    pthread_t pthread = thread->native_handle();

#ifdef __APPLE__
    // Mac OS X
    thread_affinity_policy_data_t policy = {(int) cpu_id};
    thread_port_t mach_thread = pthread_mach_thread_np(pthread);
    thread_policy_set(mach_thread, THREAD_AFFINITY_POLICY,
                      (thread_policy_t) &policy,
                      THREAD_AFFINITY_POLICY_COUNT);
#else
    // Linux
    cpu_set_t cpuset;
    CPU_ZERO(&cpuset);
    CPU_SET(cpu_id, &cpuset);
    int rc = pthread_setaffinity_np(pthread, sizeof(cpu_set_t), &cpuset);
    if (rc != 0) {
        return false;
    }
#endif
    return true;
}

} // JCpuInfo namespace

