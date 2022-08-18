
// Copyright 2020, Jefferson Science Associates, LLC.
// Subject to the terms in the LICENSE file found in the top-level directory.


#ifndef JANA2_JBACKTRACE_H
#define JANA2_JBACKTRACE_H

#include <ostream>
#include <string>
#include <execinfo.h>
#include <cxxabi.h>
#include <dlfcn.h>
#include <cstring>


inline std::ostream& make_backtrace(std::ostream& os) {

    try {
        // Generate stack trace.
        const int max_frames = 100;
        void* backtrace_buffer[max_frames];
        int frame_count = backtrace(backtrace_buffer, max_frames);
        char** symbols = backtrace_symbols(backtrace_buffer, max_frames);
        // Skip the first two frames, because those are "make_backtrace" and "JException::JException"
        for (int i=2; i<frame_count; ++i) {

            std::string name;

            // We are going to get the symbol names from dl, so that we can demangle them
            Dl_info dlinfo;
            if (dladdr(backtrace_buffer[i], &dlinfo)) { // dladdr succeeded
                if (dlinfo.dli_sname) { // dladdr found a corresponding symbol

                    int demangle_status = 0;
                    auto demangled_name = abi::__cxa_demangle(dlinfo.dli_sname, nullptr, nullptr, &demangle_status);

                    if (demangle_status == 0) { // Demangle succeeded, we have a symbol name!
                        name = std::string(demangled_name);
                    }
                    else { // Demangle failed, so we use mangled version
                        name = std::string(dlinfo.dli_sname);
                    }
                    free(demangled_name);
                }
                else { // Otherwise use the shared object name
                    name = std::string(dlinfo.dli_fname);
                }
            }
            else { // dladdr failed for some reason, so we fall back on backtrace_symbols
                name = symbols[i];
            }

            // Ignore frames where the name seems un-useful, particularly on macOS
            //if( name.find("?") != string::npos ) continue;
            //if( name == "0x0") continue;
            //if( name == "_sigtramp" ) continue;
            //if( name == "_pthread_body" ) continue;
            //if( name == "thread_start" ) continue;

            os << std::string(i+5, ' ') + "`- " << name << " (" << std::hex << backtrace_buffer[i] << std::dec << ")" << std::endl;
        }
    }
    catch (...) {
        // If this fails (e.g. bad alloc), just keep going
    }
    return os;
}


#endif //JANA2_JBACKTRACE_H
