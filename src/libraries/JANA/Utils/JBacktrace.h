
// Copyright 2020, Jefferson Science Associates, LLC.
// Subject to the terms in the LICENSE file found in the top-level directory.


#pragma once

#include <chrono>
#include <ostream>
#include <sstream>
#include <string>
#include <execinfo.h>
#include <cxxabi.h>
#include <dlfcn.h>
#include <atomic>
#include <cstring>
#include <thread>
#include <iomanip>


class JBacktrace {
    static const int MAX_FRAMES = 100;
    void* m_buffer[MAX_FRAMES];
    int m_frame_count = 0;
    int m_frames_to_omit = 0;
    std::atomic_bool m_ready {false};

public:

    void WaitForCapture() const {
        while (!m_ready.load(std::memory_order_acquire)) {
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
        }
    }

    void Capture(int frames_to_omit=0) {
        m_frame_count = backtrace(m_buffer, MAX_FRAMES);
        m_frames_to_omit = frames_to_omit;
        m_ready.store(true, std::memory_order::release);
    }

    void Format(std::ostream& os) {
        char** symbols = backtrace_symbols(m_buffer, m_frame_count);
        // Skip the first few frames, which are inevitably signal handlers, JBacktrace ctors, or JException ctors
        for (int i=m_frames_to_omit; i<m_frame_count; ++i) {

            std::string name;

            // We are going to get the symbol names from dl, so that we can demangle them
            Dl_info dlinfo;
            if (dladdr(m_buffer[i], &dlinfo)) { // dladdr succeeded
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
                    name = "???";
                }
            }
            else { // dladdr failed for some reason, so we fall back on backtrace_symbols
                name = symbols[i];
            }
            size_t offset = (size_t) m_buffer[i] - (size_t) dlinfo.dli_fbase;
            os << "  " << std::setw(2) << i-m_frames_to_omit << ": " << name << std::endl;
            os << "      " << AddrToLineInfo(dlinfo.dli_fname, offset) << std::endl;
        }
        free(symbols);
    }

    std::string AddrToLineInfo(const char* filename, size_t offset) {

        char backup_line_info[256];
        std::snprintf(backup_line_info, sizeof(backup_line_info), "%s:%zx", filename, offset);

        static bool addr2line_works = true;
        if (addr2line_works) {
            char command[256];
            std::snprintf(command, sizeof(command), "addr2line -e %s %zx", filename, offset);

            FILE* pipe = popen(command, "r");
            if (pipe) {
                // Capture stdout
                std::string line_info;
                char buffer[128];
                while (fgets(buffer, sizeof(buffer), pipe) != nullptr) {
                    line_info += buffer;
                }

                int return_code = pclose(pipe);
                if (return_code == 0) {
                    if (line_info != "??:0") {
                        return line_info;
                    }
                }
            }
            else {
                addr2line_works = false;
            }
        }
        // If addr2line failed for any reason, return the dladdr results instead
        return std::string(backup_line_info);
    }

    std::string ToString() {
        std::ostringstream oss;
        Format(oss);
        return oss.str();
    }


};


