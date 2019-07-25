//
//    File: JException.cc
// Created: Fri Oct 20 09:36:30 EDT 2017
// Creator: davidl (on Darwin harriet.jlab.org 15.6.0 i386)
//
// ------ Last repository commit info -----
// [ Date ]
// [ Author ]
// [ Source ]
// [ Revision ]
//
//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - 
// Jefferson Science Associates LLC Copyright Notice:  
// Copyright 251 2014 Jefferson Science Associates LLC All Rights Reserved. Redistribution
// and use in source and binary forms, with or without modification, are permitted as a
// licensed user provided that the following conditions are met:  
// 1. Redistributions of source code must retain the above copyright notice, this
//    list of conditions and the following disclaimer. 
// 2. Redistributions in binary form must reproduce the above copyright notice, this
//    list of conditions and the following disclaimer in the documentation and/or other
//    materials provided with the distribution.  
// 3. The name of the author may not be used to endorse or promote products derived
//    from this software without specific prior written permission.  
// This material resulted from work developed under a United States Government Contract.
// The Government retains a paid-up, nonexclusive, irrevocable worldwide license in such
// copyrighted data to reproduce, distribute copies to the public, prepare derivative works,
// perform publicly and display publicly and to permit others to do so.   
// THIS SOFTWARE IS PROVIDED BY JEFFERSON SCIENCE ASSOCIATES LLC "AS IS" AND ANY EXPRESS
// OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
// MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL
// JEFFERSON SCIENCE ASSOCIATES, LLC OR THE U.S. GOVERNMENT BE LIABLE TO LICENSEE OR ANY
// THIRD PARTES FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
// DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS
// OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
// LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
// OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
// POSSIBILITY OF SUCH DAMAGE.
//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - 

#include "JException.h"

#include <iostream>
#include <sstream>
#include <execinfo.h>
#include <cxxabi.h>
#include <dlfcn.h>
#include <cstring>

using namespace std;


JException::JException(string message) : message(std::move(message)) {

    try {
        // Generate stack trace.
        ostringstream ss;
        const int max_frames = 100;
        void* backtrace_buffer[max_frames];
        int frame_count = backtrace(backtrace_buffer, max_frames);
        char** symbols = backtrace_symbols(backtrace_buffer, max_frames);
        for (int i=0; i<frame_count; ++i) {

            string name;

            // We are going to get the symbol names from dl, so that we can demangle them
            Dl_info dlinfo;
            if (dladdr(backtrace_buffer[i], &dlinfo)) { // dladdr succeeded
                if (dlinfo.dli_sname) { // dladdr found a corresponding symbol

                    int demangle_status = 0;
                    auto demangled_name = abi::__cxa_demangle(dlinfo.dli_sname, NULL, NULL, &demangle_status);

                    if (demangle_status == 0) { // Demangle succeeded, we have a symbol name!
                        name = string(demangled_name);
                    }
                    else { // Demangle failed, so we use mangled version
                        name = string(dlinfo.dli_sname);
                    }
                    free(demangled_name);
                }
                else { // Otherwise use the shared object name
                    name = string(dlinfo.dli_fname);
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

            ss << string(i+5, ' ') + "`- " << name << " (" << hex << backtrace_buffer[i] << dec << ")" << endl;
        }
        stacktrace = ss.str();
    }
    catch (...) {
        // If this fails (e.g. bad alloc), just omit the stacktrace
    }

}


JException::~JException() = default;


/// Deprecated
std::string JException::GetMessage() {
    return message;
}


/// Convenience method for formatting complete error data
std::ostream& operator<<(std::ostream& os, JException const& ex) {
    os << "ERROR: " << ex.message << std::endl;
    os << "  Plugin:         " << ex.plugin_name << std::endl;
    os << "  Component:      " << ex.component_name << std::endl;
    os << "  Factory name:   " << ex.factory_name << std::endl;
    os << "  Factory tag:    " << ex.factory_tag << std::endl;
    os << "  Backtrace:" << std::endl << std::endl << ex.stacktrace << std::endl << std::endl;
    return os;
}


