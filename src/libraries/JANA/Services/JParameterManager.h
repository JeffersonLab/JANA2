
// Copyright 2020, Jefferson Science Associates, LLC.
// Subject to the terms in the LICENSE file found in the top-level directory.

#ifndef _JParameterManager_h_
#define _JParameterManager_h_

#include <string>
#include <algorithm>
#include <vector>
#include <map>

#include <JANA/JLogger.h>
#include <JANA/JException.h>
#include <JANA/Services/JServiceLocator.h>


struct JParameter {
    std::string name;             // A token (no whitespace, colon-prefixed), e.g. "my_plugin:use_mc"
    std::string value;            // Stringified value, e.g. "1". Can handle comma-separated vectors of strings e.g. "abc, def"
    std::string default_value;    // Optional default value, which may or may not equal value
    std::string description;      // One-liner
    bool has_default = false;     // Indicates that a default value is present. Does not indicate whether said value is in use.
    bool is_default = false;      // Indicates that we are actually using this default value.

    JParameter(std::string key, std::string val, std::string def, std::string desc, bool has_def, bool is_def)
    : name(std::move(key)),
      value(std::move(val)),
      default_value(std::move(def)),
      description(std::move(desc)),
      has_default(has_def),
      is_default(is_def) {}

    inline void SetDescription(std::string desc) { this->description = std::move(desc); }
    inline const std::string& GetKey() const { return name; }
    inline const std::string& GetValue() const { return value; }
    inline const std::string& GetDefault() const { return default_value; }
    inline bool IsDefault() const { return is_default; }
};

class JParameterManager : public JService {
public:

    JParameterManager();

    JParameterManager(const JParameterManager&);

    virtual ~JParameterManager() override;

    bool Exists(std::string name);

    JParameter* FindParameter(std::string);

    void PrintParameters(bool all = false);

    template<typename T>
    JParameter* GetParameter(std::string name, T& val);

    template<typename T>
    T GetParameterValue(std::string name);

    template<typename T>
    JParameter* SetParameter(std::string name, T val);

    template<typename T>
    JParameter* SetDefaultParameter(std::string name, T& val, std::string description = "");

    void FilterParameters(std::map<std::string,std::string> &parms, std::string filter="");

    void ReadConfigFile(std::string name);

    void WriteConfigFile(std::string name);

protected:

    template<typename T>
    T parse(const std::string& value);

    template<typename T>
    std::string stringify(T value);

    std::string to_lower(const std::string& name);

    std::map<std::string, JParameter*> m_parameters;

    JLogger m_logger;

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

    auto result = m_parameters.find(to_lower(name));
    if (result == m_parameters.end()) {
        return nullptr;
    }
    val = parse<T>(result->second->value);
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
    auto result = m_parameters.find(to_lower(name));
    if (result == m_parameters.end()) {
        throw JException("Unknown parameter \"%s\"", name.c_str());
    }
    return parse<T>(result->second->value);
}


/// @brief Sets a configuration parameter
/// @param [in] name        The parameter name
/// @param [in] val         The parameter value. This may be typed, or it may be a string.
/// @return                 Pointer to the JParameter that was either created or updated.
///                         Note that this is owned by this JParameterManager, so do not delete.
template<typename T>
JParameter* JParameterManager::SetParameter(std::string name, T val) {

    auto result = m_parameters.find(to_lower(name));

    if (result == m_parameters.end()) {
        auto* param = new JParameter {name, stringify(val), "", "", false, false};
        m_parameters[to_lower(name)] = param;
        return param;
    }
    result->second->value = stringify(val);
    result->second->is_default = false;
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

    auto result = m_parameters.find(to_lower(name));
    if (result != m_parameters.end()) {
        // We already have a value stored for this parameter
        param = result->second;

        if (!param->has_default) {
            // Our existing value is a non-default value.
            // We still want to remember the default for future conflict detection.
            param->has_default = true;
            param->default_value = stringify(val);
        }
        // else if (parse<T>(param->default_value) != val) {
        //     // Our existing value is another default, and it conflicts
        //     LOG_WARN(m_logger) << "Parameter '" << name << "' has conflicting defaults" << LOG_END;
        // }
    }
    else {
        // We are storing a value for this parameter for the first time
        auto valstr = stringify(val);
        param = new JParameter {name, valstr, valstr, std::move(description), true, true};

        // Test whether parameter is one-to-one with its string representation.
        // If not, we have a problem
        if (parse<T>(valstr) != val) {
            LOG_WARN(m_logger) << "Parameter '" << name << "' loses equality with itself after stringification" << LOG_END;
        }
        m_parameters[to_lower(name)] = param;
    }

    // Always put val through the stringification/parsing cycle to be consistent with
    // values passed in from config file, accesses from other threads
    val = parse<T>(param->value);
    return param;
}


/// @brief Logic for parsing different types in a generic way
/// @throws JException in case parsing fails.
template <typename T>
inline T JParameterManager::parse(const std::string& value) {
    std::stringstream ss(value);
    T val;
    ss >> val;
    return val;
}

/// @brief Specialization for bool
template <>
inline bool JParameterManager::parse(const std::string& value) {
    if (value == "0") return false;
    if (value == "1") return true;
    if (value == "false") return false;
    if (value == "true") return true;
    if (value == "off") return false;
    if (value == "on") return true;
    throw JException("'%s' not parseable as bool", value.c_str());
}


/// @brief Specialization for std::vector<std::string>
template<>
inline std::vector<std::string> JParameterManager::parse(const std::string& value) {
    std::stringstream ss(value);
    std::vector<std::string> result;
    std::string s;
    while (getline(ss, s, ',')) {
        result.push_back(s);
    }
    return result;
}

/// @brief Logic for stringifying different types in a generic way
template <typename T>
inline std::string JParameterManager::stringify(T value) {
    std::stringstream ss;
    ss << value;
    return ss.str();
}

/// Specialization for std::vector<std::string>
template<>
inline std::string JParameterManager::stringify(std::vector<std::string> value) {

    std::stringstream ss;
    size_t len = value.size();
    for (size_t i = 0; i+1 < len; ++i) {
        ss << value[i];
        ss << ",";
    }
    if (len != 0) {
        ss << value[len-1];
    }
    return ss.str();
}

#endif // _JParameterManager_h_

