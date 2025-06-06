
// Copyright 2020, Jefferson Science Associates, LLC.
// Subject to the terms in the LICENSE file found in the top-level directory.

#pragma once

#include <JANA/Utils/JBacktrace.h>
#include <sstream> // This is only here in order to not break halld_recon
#include <string>

/// JException is a data object which attaches JANA-specific context information to a generic exception.
/// As it unwinds the call stack, different exception handlers may add or change information as they see fit.
/// It does not use getters and setters, because they are not needed, because there is no invariant.
struct JException : public std::exception {
public:

    /// Basic constructor
    explicit JException(std::string message = "Unknown exception") : message(std::move(message)) {
        backtrace.Capture(2);
    }

    virtual ~JException() = default;


    /// Constructor with printf-style formatting
    template<typename... Args>
    explicit JException(std::string message, Args... args);


    std::string GetMessage() {
        return message;
    }

    std::string GetStackTrace() {
        return backtrace.ToString();
    }

    const char* what() const noexcept {
        return message.c_str();
    }

    /// Convenience method for formatting complete error data
    inline friend std::ostream& operator<<(std::ostream& os, JException const& ex) {
        os << "JException" << std::endl;
        if (ex.exception_type.length() != 0) {
            os << "  Type:     " << ex.exception_type << std::endl;
        }
        if (ex.message.length() != 0) {
            os << "  Message:  " << ex.message << std::endl;
        }
        if (ex.function_name.length() != 0) {
            os << "  Function: " << ex.function_name << std::endl;
        }
        if (ex.type_name.length() != 0) {
            os << "  Class:    " << ex.type_name << std::endl;
        }
        if (ex.instance_name.length() != 0) {
            os << "  Instance: " << ex.instance_name << std::endl;
        }
        if (ex.plugin_name.length() != 0) {
            os << "  Plugin:   " << ex.plugin_name << std::endl;
        }
        if (ex.show_stacktrace) {
            ex.backtrace.WaitForCapture();
            ex.backtrace.ToString();
            os << "  Backtrace:" << std::endl << std::endl << ex.backtrace.ToString();
        }
        return os;
    }

    std::string exception_type;
    std::string message;
    std::string plugin_name;
    std::string type_name;
    std::string function_name;
    std::string instance_name;
    JBacktrace backtrace;
    std::exception_ptr nested_exception;
    bool show_stacktrace=true;

};


/// Constructor with convenient printf-style formatting.
/// Uses variadic templates (although it is slightly overkill) because variadic functions are frowned on now.
template<typename... Args>
JException::JException(std::string format_str, Args... args) {
    char cmess[1024];
    snprintf(cmess, 1024, format_str.c_str(), args...);
    message = cmess;
    backtrace.Capture(2);
}


