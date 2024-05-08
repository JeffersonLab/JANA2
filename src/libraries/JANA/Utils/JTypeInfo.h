
// Copyright 2020, Jefferson Science Associates, LLC.
// Subject to the terms in the LICENSE file found in the top-level directory.


#ifndef _JTypeInfo_h_
#define _JTypeInfo_h_

#include <cstdint>
#include <cxxabi.h>
#include <string>

namespace JTypeInfo {


template <typename, typename=void>
struct is_parseable : std::false_type {};

template <typename T>
struct is_parseable<T, std::void_t<decltype(std::declval<std::istream>() >> std::declval<T&>())>> : std::true_type {};

template <typename, typename=void>
struct is_serializable : std::false_type {};

template <typename T>
struct is_serializable<T, std::void_t<decltype(std::declval<std::ostream>() << std::declval<T>())>> : std::true_type {};


template<typename T>
std::string demangle() {

    /// Return the demangled name (if available) for the type the template
    /// is based. Call it like this:
    ///   cout << demangle<MyType>() << endl;
    int status = -1;
    auto cstr = abi::__cxa_demangle(typeid(T).name(), NULL, NULL, &status);
    std::string type(cstr);
    free(cstr);
    if (status != 0) type = typeid(T).name();
    return type;
}


inline std::string demangle_current_exception_type() {

    int status = -1;
    auto cstr = abi::__cxa_demangle(abi::__cxa_current_exception_type()->name(), NULL, NULL, &status);
    std::string type(cstr);
    free(cstr);
    if (status != 0) type = abi::__cxa_current_exception_type()->name();
    return type;
}

/// Macro for conveniently turning a variable name into a string. This is used by JObject::Summarize
/// in order to play nicely with refactoring tools. Because the symbol is picked up by the
/// preprocessor and not the compiler, no demangling is necessary.
#define NAME_OF(T) #T


/// Macro for extracting the typename of the current class as a std::string.
/// This is meant to be used like this:
/// `JEventProcessor::SetName(NAME_OF_THIS);`
#define NAME_OF_THIS JTypeInfo::demangle<decltype(*this)>()


template<typename T>
const char* builtin_typename() {
    /// Function template for stringifying builtin typenames.
    /// This provides some normalization, i.e. both ulongs and uint64_t's get converted to "ulong"

    if (typeid(T) == typeid(int) || typeid(T) == typeid(int32_t)) {
        return "int";
    } else if (typeid(T) == typeid(unsigned int) || typeid(T) == typeid(uint32_t)) {
        return "uint";
    } else if (typeid(T) == typeid(long) || typeid(T) == typeid(int64_t)) {
        return "long";
    } else if (typeid(T) == typeid(unsigned long) || typeid(T) == typeid(uint64_t)) {
        return "ulong";
    } else if (typeid(T) == typeid(short) || typeid(T) == typeid(int16_t)) {
        return "short";
    } else if (typeid(T) == typeid(unsigned short) || typeid(T) == typeid(uint16_t)) {
        return "ushort";
    } else if (typeid(T) == typeid(float)) {
        return "float";
    } else if (typeid(T) == typeid(double)) {
        return "double";
    } else if (typeid(T) == typeid(std::string) || typeid(T) == typeid(const char*) || typeid(T) == typeid(char*)) {
        return "string";
    } else {
        return "unknown";
    }
};

inline std::string to_string_with_si_prefix(float val) {

    /// Return the value as a string with the appropriate latin unit prefix appended.
    /// Values returned are: "G", "M", "k", "", "u", and "m" for values of "val" that are:
    /// >1.5E9, >1.5E6, >1.5E3, <1.0E-7, <1.0E-4, 1.0E-1, respectively.

    const char* units = "";
    if (val > 1.5E9) {
        val /= 1.0E9;
        units = "G";
    } else if (val > 1.5E6) {
        val /= 1.0E6;
        units = "M";
    } else if (val > 1.5E3) {
        val /= 1.0E3;
        units = "k";
    } else if (val < 1.0E-7) {
        units = "";
    } else if (val < 1.0E-4) {
        val /= 1.0E6;
        units = "u";
    } else if (val < 1.0E-1) {
        val /= 1.0E3;
        units = "m";
    }
    char str[256];
    snprintf(str, 256, "%3.1f %s", val, units);
    return std::string(str);
}


} // namespace JTypeInfo
#endif
