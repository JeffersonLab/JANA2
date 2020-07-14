
// Copyright 2020, Jefferson Science Associates, LLC.
// Subject to the terms in the LICENSE file found in the top-level directory.

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
        os << "Exception: " << ex.message << std::endl << std::endl;
        if (ex.plugin_name.length() != 0) {
            os << "  Plugin:         " << ex.plugin_name << std::endl;
        }
        if (ex.component_name.length() != 0) {
            os << "  Component:      " << ex.component_name << std::endl;
        }
        if (ex.factory_name.length() != 0) {
            os << "  Factory name:   " << ex.factory_name << std::endl;
            os << "  Factory tag:    " << ex.factory_tag << std::endl;
        }
        if (ex.stacktrace.length() != 0) {
            os << "  Backtrace:" << std::endl << std::endl << ex.stacktrace << std::endl << std::endl;
        }
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
JException::JException(std::string format_str, Args... args) {
    char cmess[1024];
    snprintf(cmess, 1024, format_str.c_str(), args...);
    message = cmess;
}

#endif // _JException_h_

