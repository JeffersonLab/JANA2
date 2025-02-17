
// Copyright 2020, Jefferson Science Associates, LLC.
// Subject to the terms in the LICENSE file found in the top-level directory.

#pragma once

#include <cstdio>
#include <limits>
#include <sstream>
#include <string>
#include <algorithm>
#include <vector>
#include <array>
#include <map>
#include <cmath>
#include <iomanip>
#include <cstring>

#include <JANA/JLogger.h>
#include <JANA/JException.h>
#include <JANA/Services/JServiceLocator.h>

class JParameter {

    std::string m_name;             // A token (no whitespace, colon-prefixed), e.g. "my_plugin:use_mc"
    std::string m_value;            // Stringified value, e.g. "1". Can handle comma-separated vectors of strings e.g. "abc, def"
    std::string m_default_value;    // Optional default value, which may or may not equal value
    std::string m_description;      // One-liner

    bool m_has_default = false;     // Indicates that a default value is present. Does not indicate whether said value is in use.
                                    //   We need this because "" is a valid default value.
    bool m_is_default = false;      // Indicates that we are actually using this default value.
                                    //   We need this because we want to know if the user provided a value that happened to match the default value.
    bool m_is_deprecated = false;   // This lets us print a warning if a user provides a deprecated parameter,
                                    //   It also lets us suppress printing this parameter in the parameter list.
    bool m_is_advanced = false;       // Some JANA parameters control internal details that aren't part of the contract between JANA and its users.
                                    //   We want to differentiate these from the parameters that users are meant to control and understand.
    bool m_is_used = false;         // If a parameter hasn't been used, it probably contains a typo, and we should warn the user.

    bool m_is_conflicted = false;   // Whether or not this parameter has been registered with inconsistent default values

public:

    JParameter(std::string key, std::string value, std::string defaultValue, std::string description, bool hasDefault, bool isDefault)
    : m_name(std::move(key)),
      m_value(std::move(value)),
      m_default_value(std::move(defaultValue)),
      m_description(std::move(description)),
      m_has_default(hasDefault),
      m_is_default(isDefault) {}

    inline const std::string& GetKey() const { return m_name; }
    inline const std::string& GetValue() const { return m_value; }
    inline const std::string& GetDefault() const { return m_default_value; }
    inline const std::string& GetDescription() const { return m_description; }

    inline bool HasDefault() const { return m_has_default; }
    inline bool IsDefault() const { return m_is_default; }
    inline bool IsAdvanced() const { return m_is_advanced; }
    inline bool IsUsed() const { return m_is_used; }
    inline bool IsDeprecated() const { return m_is_deprecated; }
    inline bool IsConflicted() const { return m_is_conflicted; }

    inline void SetKey(std::string key) { m_name = std::move(key); }
    inline void SetValue(std::string val) { m_value = std::move(val); }
    inline void SetDefault(std::string defaultValue) { m_default_value = std::move(defaultValue); }
    inline void SetDescription(std::string desc) { m_description = std::move(desc); }
    inline void SetHasDefault(bool hasDefault) { m_has_default = hasDefault; }
    inline void SetIsDefault(bool isDefault) { m_is_default = isDefault; }
    inline void SetIsAdvanced(bool isHidden) { m_is_advanced = isHidden; }
    inline void SetIsUsed(bool isUsed) { m_is_used = isUsed; }
    inline void SetIsDeprecated(bool isDeprecated) { m_is_deprecated = isDeprecated; }
    inline void SetIsConflicted(bool isConflicted) { m_is_conflicted = isConflicted; }

};

class JParameterManager : public JService {
public:

    JParameterManager();

    JParameterManager(const JParameterManager&);

    ~JParameterManager() override;

    inline void SetLogger(JLogger logger) { m_logger = logger; }

    bool Exists(std::string name);

    JParameter* FindParameter(std::string);

    void PrintParameters();
    
    void PrintParameters(int verbosity, int strictness);

    std::map<std::string, JParameter*> GetAllParameters();

    template<typename T>
    JParameter* GetParameter(std::string name, T& val);

    template<typename T>
    T GetParameterValue(std::string name);

    template<typename T>
    JParameter* SetParameter(std::string name, T val);

    template<typename T>
    JParameter* SetDefaultParameter(std::string name, T& val, std::string description = "");

    template <typename T>
    T RegisterParameter(std::string name, const T default_val, std::string description = "");

    void FilterParameters(std::map<std::string,std::string> &parms, std::string filter="");

    void ReadConfigFile(std::string name);

    void WriteConfigFile(std::string name);

    template<typename T>
    static inline void Parse(const std::string& value, T& out);

    template<typename T>
    static inline void Parse(const std::string& value, std::vector<T>& out);

    template<typename T, size_t arrSize>
    static inline void Parse(const std::string& value, std::array<T,arrSize>& out); 

    template<size_t arrSize>
    static inline void Parse(const std::string& value, std::array<std::string,arrSize>& out);


    /*
    Template specialization done for double and float numbers in order to match the 
    precision of the number with the string
    */
    template<typename T>
    static std::string Stringify(const T& value);

    template<typename T>
    static std::string Stringify(const std::vector<T>& values);

    template<typename T,size_t N>
    static std::string Stringify(const std::array<T,N>& values);
 
    template<size_t N>
    static std::string Stringify(const std::array<std::string,N>& values);

    template<typename T>
    static bool Equals(const T& lhs, const T& rhs);


    static std::string ToLower(const std::string& name);

    JLogger GetLogger(const std::string& prefix);

private:

    std::map<std::string, JParameter*> m_parameters;

    int m_strictness = 1;
    int m_verbosity = 1;

    std::mutex m_mutex;
};



/// @brief Retrieves a JParameter and stores its value
///
/// @param [in] name    The name of the parameter to retrieve
/// @param [out] val    Reference to where the parameter value should be stored
/// @returns            The corresponding JParameter representation, which provides us with
///                     additional information such as the default value, or nullptr if not found.
///
template<typename T>
JParameter* JParameterManager::GetParameter(std::string name, T& val) {

    auto result = m_parameters.find(ToLower(name));
    if (result == m_parameters.end()) {
        return nullptr;
    }
    Parse(result->second->GetValue(),val);
    result->second->SetIsUsed(true);
    return result->second;
}


/// @brief Retrieves a parameter value
///
/// @param [in] name    The name of the parameter to retrieve
/// @returns            The parameter value
/// @throws JException  in case the parameter is not found
///
template<typename T>
T JParameterManager::GetParameterValue(std::string name) {
    T t;
    auto result = m_parameters.find(ToLower(name));
    if (result == m_parameters.end()) {
        throw JException("Unknown parameter \"%s\"", name.c_str());
    }
    result->second->SetIsUsed(true);
    Parse(result->second->GetValue(),t);
    return t;
}


/// @brief Sets a configuration parameter
/// @param [in] name        The parameter name
/// @param [in] val         The parameter value. This may be typed, or it may be a string.
/// @return                 Pointer to the JParameter that was either created or updated.
///                         Note that this is owned by this JParameterManager, so do not delete.
template<typename T>
JParameter* JParameterManager::SetParameter(std::string name, T val) {

    auto result = m_parameters.find(ToLower(name));

    if (result == m_parameters.end()) {
        auto* param = new JParameter {name, Stringify(val), "", "", false, false};
        m_parameters[ToLower(name)] = param;
        return param;
    }
    result->second->SetValue(Stringify(val));
    result->second->SetIsDefault(false);
    return result->second;
}



/// @brief Retrieve a configuration parameter, if necessary creating it with the provided default value
///
/// @param [in] name            The parameter name. Must not contain spaces.
/// @param [in,out] val         Reference to a variable which has been set to the desired default value.
/// @param [in] description     Optional description, e.g. units, set or range of valid values, etc.
///
/// @details Upon entry, the value in "val" should be set to the desired default value. It
/// will be overwritten if a value for the parameter already exists because
/// it was given by the user either on the command line or in a configuration
/// file. If the parameter does not already exist, it is created and its value set
/// to that of "val". Upon exit, "val" will always contain the value that should be used
/// for event processing.
///
/// If a parameter with the given name already exists, it will be checked to see
/// if the parameter already has a default value assigned (this is kept separate
/// from the actual value of the parameter used and is maintained purely for
/// bookkeeping purposes). If it does not have a default value, then the value
/// of "val" upon entry is saved as the default. If it does have a default, then
/// the value of the default is compared to the value of "val" upon entry. If the
/// two do not match, then a warning message is printed to indicate to the user
/// that two different default values are being set for this parameter.
///
/// Parameters specified on the command line using the "-Pkey=value" syntax will
/// not have a default value at the time the parameter is created.
///
template<typename T>
JParameter* JParameterManager::SetDefaultParameter(std::string name, T& val, std::string description) {

    std::lock_guard<std::mutex> lock(m_mutex);
    JParameter* param = nullptr;
    T t;
    auto result = m_parameters.find(ToLower(name));
    if (result != m_parameters.end()) {
        // We already have a value stored for this parameter
        param = result->second;

        if (!param->HasDefault()) {
            // We don't have a default value yet. Our existing value is a non-default value.
            param->SetHasDefault(true);
            param->SetDefault(Stringify(val));
            param->SetDescription(std::move(description));
        }
        else {
            // We tried to set the same default parameter twice. This is fine; this happens all the time
            // because we construct lots of copies of JFactories, each of which calls SetDefaultParameter on its own.
            // However, we still want to warn the user if the same parameter was declared with different values.
            Parse(param->GetDefault(),t);
            if (!Equals(val, t)) {
                if (!param->IsConflicted()) {
                    // Only show this warning once per parameter
                    LOG_WARN(m_logger) << "Parameter '" << name << "' has conflicting defaults: '"
                                    << Stringify(val) << "' vs '" << param->GetDefault() << "'"
                                    << LOG_END;
                    param->SetIsConflicted(true);
                }
                // If we tried to set the same default parameter twice with different values, and there is no
                // existing non-default value, we remember the _latest_ default value. This way, we return the
                // default provided by the caller, instead of the default provided by the mysterious interloper.
                param->SetDefault(Stringify(val));
            }
            if (param->IsDefault()) {
                param->SetValue(Stringify(val)); // Use _latest_ default value
            }
        }
    }
    else {
        // We are storing a value for this parameter for the first time
        auto valstr = Stringify(val);
        param = new JParameter {name, valstr, valstr, std::move(description), true, true};

        // Test whether parameter is one-to-one with its string representation.
        // If not, warn the user they may have a problem.
        
        Parse(valstr,t);
        if (!Equals(val, t)) {
            LOG_WARN(m_logger) << "Parameter '" << name << "' with value '" << valstr
                               << "' loses equality with itself after stringification" << LOG_END;
        }
        m_parameters[ToLower(name)] = param;
    }

    // Always put val through the stringification/parsing cycle to be consistent with
    // values passed in from config file, accesses from other threads
    Parse(param->GetValue(),t);
    val = t;
    param->SetIsUsed(true);
    return param;
}

/// @brief Retrieve a configuration parameter, if necessary creating it with the provided default value.
///
/// @param [in] name            The parameter name. Must not contain spaces.
/// @param [in,out] default_val Value to set the desired default value to (returned if no value yet set for parameter).
/// @param [in] description     Optional description, e.g. units, set or range of valid values, etc.
/// @return                     Current value of parameter
///
/// @details This is a convenience method that wraps SetDefaultParameter. The difference
/// is that this has the default passed by value (not by reference) and the value of the parameter is returned
/// by value. This allows a slightly different form for declaring configuration parameters with a default value.
/// e.g.
///     auto thresh = jpp->RegisterParameter("SystemA:threshold", 1.3, "threshold in MeV");
///
template <typename T>
inline T JParameterManager::RegisterParameter(std::string name, const T default_val, std::string description){
    T val = default_val;
    SetDefaultParameter(name.c_str(), val, description);
    return val;
}


#if __cplusplus >= 201703L
/// @brief Basic implementation of Parse for C++17 and newer. Provides a helpful error message when attempting to parse a type that doesn't come with a stream operator.
template <typename T>
void JParameterManager::Parse(const std::string& s, T& out) {
    constexpr bool parseable = JTypeInfo::is_parseable<T>::value;
    static_assert(parseable, "Type is not automatically parseable by JParameterManager. To use, provide a template specialization for JParameterManager::Parse(std::string in, T& out).");
    if constexpr (parseable) {
        std::stringstream ss(s);
        ss >> out;
    }
}
#else
/// @brief Basic implementation of Parse for C++14 and earlier.
template <typename T>
void JParameterManager::Parse(const std::string& s, T& out) {
    std::stringstream ss(s);
    ss >> out;
}
#endif


/// @brief Specialization for string. 
/// The stream operator is not only redundant here, but it also splits the string (see Issue #191)
template <>
inline void JParameterManager::Parse(const std::string& value, std::string& out) {
    out = value;
}


/// @brief Specialization for bool
template <>
inline void JParameterManager::Parse(const std::string& value, bool& val) {
    if (value == "0" || value =="false" || value == "off") val = false;
    else if (value == "1" || value == "true" || value == "on") val = true;
    else throw JException("'%s' not parseable as bool", value.c_str());
}

// @brief Template to parse a string and return in an array
template <typename T, size_t N>
inline void JParameterManager::Parse(const std::string& value,std::array<T,N> &val) {
    std::string s;
    std::stringstream ss(value);
    int indx = 0;
    while (getline(ss, s, ',')) {
        T t;
        Parse(s, t);
        val[indx++]= t;
    }
}

// @brief Template to parse a string and return in an array of strings
template <size_t N>
inline void JParameterManager::Parse(const std::string& value,std::array<std::string,N> &val) {
    std::string s;
    std::stringstream ss(value);
    std::string temp;   // creating a temp var allows us to store the string s where the escape character was used
    int indx = 0;
    while (getline(ss, s, ',')) {
        std::string t;
        if (s.back() == '\\') {
            s.pop_back();
            temp += s + ',';
            continue;
        }
        if (!temp.empty()) {
            Parse(temp + s, t);
            val[indx++]= t;
            temp.clear();
        }
        else {
            Parse(s, t);
            val[indx++]= t;
        }
    }
}

/// @brief Specialization for std::vector<std::string>
template <typename T>
inline void JParameterManager::Parse(const std::string& value, std::vector<T> &val) {
    std::stringstream ss(value);
    std::string s;
    val.clear(); // clearing the input vector to ensure no dulication which can be caused due to val.push_back(t);
    while (getline(ss, s, ',')) {
        T t;
        Parse(s, t);
        val.push_back(t);
    }
}

/// @brief Specialization for std::vector<std::string> with escape commas
template <>
inline void JParameterManager::Parse(const std::string& value, std::vector<std::string> &val) {
    std::stringstream ss(value);
    std::string s;
    val.clear(); // clearing the input vector to ensure no dulication which can be caused due to val.push_back(t);
    std::string temp;   // creating a temp var allows us to store the string s where the escape character was used
    while (getline(ss, s, ',')) {
        std::string t;
        if (s.back() == '\\') {
            s.pop_back();
            temp += s + ',';
            continue;
        }
        if (!temp.empty()) {
            Parse(temp + s, t);
            val.push_back(t);
            temp.clear();
        }
        else {
            Parse(s, t);
            val.push_back(t);
        }
    }
}

/// @brief Specialization for JLogger::Level enum
template <>
inline void JParameterManager::Parse(const std::string& in, JLogger::Level& out) {
    std::string token(in);
    std::transform(in.begin(), in.end(), token.begin(), ::tolower);
    if (std::strcmp(token.c_str(), "trace") == 0) { 
        out = JLogger::Level::TRACE;
    }
    else if (std::strcmp(token.c_str(), "debug") == 0) { 
        out = JLogger::Level::DEBUG;
    }
    else if (std::strcmp(token.c_str(), "info") == 0) { 
        out = JLogger::Level::INFO;
    }
    else if (std::strcmp(token.c_str(), "warn") == 0) { 
        out = JLogger::Level::WARN;
    }
    else if (std::strcmp(token.c_str(), "error") == 0) { 
        out = JLogger::Level::ERROR;
    }
    else if (std::strcmp(token.c_str(), "fatal") == 0) { 
        out = JLogger::Level::FATAL;
    }
    else if (std::strcmp(token.c_str(), "off") == 0) { 
        out = JLogger::Level::OFF;
    }
    else {
        throw JException("Unable to parse log level: '%s'. Options are: TRACE, DEBUG, INFO, WARN, ERROR, FATAL, OFF", in.c_str());
    }
}

#if __cplusplus >= 201703L
/// @brief Basic implementation of Stringify for C++17 and newer. Provides a helpful error message when attempting to stringify a type that doesn't come with a stream operator.
template <typename T>
inline std::string JParameterManager::Stringify(const T& value) {
    constexpr bool serializable = JTypeInfo::is_serializable<T>::value;
    static_assert(serializable, "Type is not automatically serializable by JParameterManager. To use, provide a template specialization for JParameterManager::Stringify(const T& val) -> std::string.");
    if constexpr (serializable) {
        std::stringstream ss;
        ss << value;
        return ss.str();
    }
}
#else
/// @brief Basic implementation of Parse for C++14 and earlier.
template <typename T>
inline std::string JParameterManager::Stringify(const T& value) {
    std::stringstream ss;
    ss << value;
    return ss.str();
}
#endif


template <>
inline std::string JParameterManager::Stringify(const float& value) {
    std::stringstream ss;
    // Use enough precision to make sure that the "float -> string -> float" roundtrip is exact. 
    // The stringstream << operator is smart enough to give us a clean representation when one is available.
    ss << std::setprecision(std::numeric_limits<float>::max_digits10) << value;
    return ss.str();
}
template <>
inline std::string JParameterManager::Stringify(const double& value) {
    std::stringstream ss;
    // Use enough precision to make sure that the "double -> string -> double" roundtrip is exact. 
    // The stringstream << operator is smart enough to give us a clean representation when one is available.
    ss << std::setprecision(std::numeric_limits<double>::max_digits10) << value;
    return ss.str();
}
template <>
inline std::string JParameterManager::Stringify(const long double& value) {
    std::stringstream ss;
    // Use enough precision to make sure that the "long double -> string -> long double" roundtrip is exact. 
    // The stringstream << operator is smart enough to give us a clean representation when one is available.
    ss << std::setprecision(std::numeric_limits<long double>::max_digits10) << value;
    return ss.str();
}

// @brief Specialization for strings. The stream operator is not only redundant here, but it also splits the string (see Issue #191)
template <>
inline std::string JParameterManager::Stringify(const std::string& value) {
    return value;
}


template <typename T>
inline std::string JParameterManager::Stringify(const std::vector<T> &values) {
    std::stringstream ss;
    size_t len = values.size();
    for (size_t i = 0; i+1 < len; ++i) {
        ss << values[i];
        ss << ",";
    }
    if (len != 0) {
        ss << values[len-1];
    }
    return ss.str();
}


template <>
inline std::string JParameterManager::Stringify(const std::vector<std::string> &values) {
    size_t len = values.size();
    std::stringstream ssv;
    for (size_t i = 0; i < len; ++i) {
        if (values[i].find(',') != std::string::npos) {
            std::stringstream ss(values[i]);
            std::string s;
            std::string s_end = values[i].substr(values[i].rfind(',')+1);
            while (getline(ss, s, ',')) {
                if (s == s_end) {
                    ssv << s;
                    if (i != len-1) {
                        ssv << ',';
                    }
                    break;
                }
                ssv << s;
                if (s.back() != '\\') {
                    ssv << '\\';
                }
                ssv << ',';
                continue;
            }
        } else {
            ssv << values[i];
            if (i != len-1) {
                ssv << ',';
            }
        }
    }
    return ssv.str();
}


// @brief Template for generically stringifying an array
template <typename T, size_t N>
inline std::string JParameterManager::Stringify(const std::array<T,N> &values) {
    std::stringstream ss;
    size_t len = values.size();
    for (size_t i = 0; i+1 < N; ++i) {
        ss << values[i];
        ss << ",";
    }
    if (len != 0) {
        ss << values[len-1];
    }
    return ss.str();
}


template <size_t N>
inline std::string JParameterManager::Stringify(const std::array<std::string,N> &values) {
    size_t len = values.size();
    std::stringstream ssv;
    for (size_t i = 0; i < len; ++i) {
        std::stringstream ss(values[i]);
        std::string s;
        std::string temp;
        while (getline(ss, s, ',')) {
            if (s.back() == '\\') {
                s.pop_back();
                temp += s + ',';
                continue;
            }
            if (!temp.empty()) {
                ssv << temp + s;
                temp.clear();
            }
            else {
                ssv << s;
            }
        }
    }
    return ssv.str();
}

// @brief Equals() is called internally by SetDefaultParameter. This allows the user to define equivalence relations on custom data types so that
// they can silence any spurious "loses equality with itself after stringification" warnings. Generally you should try specializing Stringify and Parse
// first to normalize your data representation.
template <typename T>
inline bool JParameterManager::Equals(const T& lhs, const T& rhs) {
    return lhs == rhs;
}




