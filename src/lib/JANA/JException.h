//
//    File: JException.h
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
//
// Description:
//
//
//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - 
#ifndef _JException_h_
#define _JException_h_

#include <JANA/Utils/JBacktrace.h>
#include <string>
#include <sstream>

/// JException is a data object which attaches JANA-specific context information to a generic exception.
/// As it unwinds the call stack, different exception handlers may add or change information as they see fit.
/// It does not use getters and setters, because they are not needed, because there is no invariant.
struct JException : public std::exception {
public:

    /// Basic constructor
    explicit JException(std::string message = "Unknown exception") : message(std::move(message))
    {
        std::ostringstream ss;
        make_backtrace(ss);
        stacktrace = ss.str();
    }

    virtual ~JException() = default;


    /// Constructor with printf-style formatting
    template<typename... Args>
    explicit JException(std::string message, Args... args);


    // Deprecated
    std::string GetMessage() {
        return message;
    }

    const char* what() const noexcept {
        return message.c_str();
    }

    /// Convenience method for formatting complete error data
    inline friend std::ostream& operator<<(std::ostream& os, JException const& ex) {
        os << "ERROR: " << ex.message << std::endl;
        os << "  Plugin:         " << ex.plugin_name << std::endl;
        os << "  Component:      " << ex.component_name << std::endl;
        os << "  Factory name:   " << ex.factory_name << std::endl;
        os << "  Factory tag:    " << ex.factory_tag << std::endl;
        os << "  Backtrace:" << std::endl << std::endl << ex.stacktrace << std::endl << std::endl;
        return os;
    }

    std::string message;
    std::string plugin_name;
    std::string component_name;
    std::string factory_name;
    std::string factory_tag;
    std::string stacktrace;
    std::exception_ptr nested_exception;

};


/// Constructor with convenient printf-style formatting.
/// Uses variadic templates (although it is slightly overkill) because variadic functions are frowned on now.
template<typename... Args>
JException::JException(std::string message, Args... args) {
    char cmess[1024];
    snprintf(cmess, 1024, message.c_str(), args...);
    message = cmess;
}

#endif // _JException_h_

