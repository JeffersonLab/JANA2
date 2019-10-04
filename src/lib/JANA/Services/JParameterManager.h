//
//    File: JParameterManager.h
// Created: Thu Oct 12 08:16:11 EDT 2017
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


#ifndef _JParameterManager_h_
#define _JParameterManager_h_

#include <string>
#include <algorithm>
#include <map>

#include <JANA/JLogger.h>
#include <JANA/JException.h>


struct JParameter {
    std::string name;
    std::string value;
    std::string default_value;
    std::string description;
    bool has_default;
};

class JParameterManager {
public:

    JParameterManager();

    virtual ~JParameterManager();

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

    void ReadConfigFile(std::string name);

    void WriteConfigFile(std::string name);

protected:

    template<typename T>
    T parse(std::string value);

    template<typename T>
    std::string stringify(T value);

    std::string to_lower(std::string& name);

    std::map<std::string, JParameter*> m_parameters;

    JLogger m_logger;
};



template<typename T>
JParameter* JParameterManager::GetParameter(std::string name, T& val) {

    auto result = m_parameters.find(to_lower(name));
    if (result == m_parameters.end()) {
        return nullptr;
    }
    val = parse<T>(result->second->value);
    return result->second;
}


template<typename T>
T JParameterManager::GetParameterValue(std::string name) {
    auto result = m_parameters.find(to_lower(name));
    if (result == m_parameters.end()) {
        throw JException("Unknown parameter \"%s\"", name.c_str());
    }
    return parse<T>(result->second->value);
}


template<typename T>
JParameter* JParameterManager::SetParameter(std::string name, T val) {

    auto result = m_parameters.find(to_lower(name));

    if (result == m_parameters.end()) {
        auto* param = new JParameter {name, stringify(val), "", "", false};
        m_parameters[to_lower(name)] = param;
        return param;
    }
    result->second->value = stringify(val);
    return result->second;
}


template<typename T>
JParameter* JParameterManager::SetDefaultParameter(std::string name, T& val, std::string description) {
    /// Retrieve a configuration parameter, creating it if necessary.
    ///
    /// Upon entry, the value in "val" should be set to the desired default value. It
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
    /// This should be called after the JApplication object has been initialized so
    /// that parameters can be created from any command line options the user may specify.

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
        else if (parse<T>(param->default_value) != val) {
            // Our existing value is another default, and it conflicts
            throw JException("Conflicting defaults for parameter %s", name.c_str());
        }
    }
    else {
        // We are storing a value for this parameter for the first time
        auto valstr = stringify(val);
        param = new JParameter {name, valstr, valstr, std::move(description), true};

        // Test whether parameter is one-to-one with its string representation.
        // If not, we have a problem
        if (parse<T>(valstr) != val) {
            throw JException("Parameter %s loses equality with itself after stringification");
        }
        m_parameters[to_lower(name)] = param;
    }

    // Always put val through the stringification/parsing cycle to be consistent with
    // values passed in from config file, accesses from other threads
    val = parse<T>(param->value);
    return param;
}


// Logic for parsing and stringifying different types

template <typename T>
inline T JParameterManager::parse(std::string value) {
    std::stringstream ss(value);
    T val;
    ss >> val;
    return val;
}

/// Specialization for std::vector<std::string>
template<>
inline std::vector<std::string> JParameterManager::parse(std::string value) {
    std::stringstream ss(value);
    std::vector<std::string> result;
    std::string s;
    while (getline(ss, s, ',')) {
        result.push_back(s);
    }
    return result;
}

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

