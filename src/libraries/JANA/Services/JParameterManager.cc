
// Copyright 2020, Jefferson Science Associates, LLC.
// Subject to the terms in the LICENSE file found in the top-level directory.

#include "JParameterManager.h"
#include "JANA/JLogger.h"

#include <sstream>
#include <vector>
#include <string>
#include <fstream>
#include <cstring>
#include <JANA/Utils/JTablePrinter.h>

using namespace std;


/// @brief Default constructor
JParameterManager::JParameterManager() {
    SetLoggerName("jana");
}

/// @brief Copy constructor
/// @details Does a deep copy of the JParameter objects to avoid double frees.
JParameterManager::JParameterManager(const JParameterManager& other) {

    m_logger = other.m_logger;
    // Do a deep copy of contained JParameters to avoid double frees
    for (const auto& param : other.m_parameters) {
        m_parameters.insert({param.first, new JParameter(*param.second)});
    }
}

/// @brief Destructor
/// @details Destroys all contained JParameter objects.
JParameterManager::~JParameterManager() {
    for (const auto& p : m_parameters) delete p.second;
    m_parameters.clear();
}


/// @brief Converts a parameter name to all lowercase
///
/// @details  When accessing the m_parameters map, strings are always converted to
///           lower case. This makes configuration parameters case-insensitive in effect,
///           while still preserving the user's choice of case in the parameter name.
std::string JParameterManager::ToLower(const std::string& name) {
    std::string tmp(name);
    std::transform(tmp.begin(), tmp.end(), tmp.begin(), ::tolower);
    return tmp;
}


/// @brief Test whether a parameter of some name exists
///
/// @param [in] name    the parameter name
/// @return             whether that parameter was found
bool JParameterManager::Exists(string name) {
    return m_parameters.find(ToLower(name)) != m_parameters.end();
}


/// @brief Retrieves a JParameter object of some name
///
/// @param [in] name    the parameter name
/// @return             a pointer to the JParameter.
///
/// @note The JParameter pointer is still owned by the JParameterManager, so don't delete it.
JParameter* JParameterManager::FindParameter(std::string name) {
    auto result = m_parameters.find(ToLower(name));
    if (result == m_parameters.end()) {
        return nullptr;
    }
    return result->second;
}

void JParameterManager::PrintParameters() {
    // In an ideal world, these parameters would be declared in Init(). However, we always have the chicken-and-egg problem to contend with
    // when initializing ParameterManager and other Services. As long as PrintParameters() gets called at the end of JApplication::Initialize(),
    // this is fine.
    SetDefaultParameter("jana:parameter_verbosity", m_verbosity, "0: Don't show parameters table\n"
                                                                 "1: Show user-provided parameters only\n"
                                                                 "2: Show defaulted parameters\n"
                                                                 "3: Show defaulted advanced parameters");
    
    SetDefaultParameter("jana:parameter_strictness", m_strictness, "0: Ignore unused parameters\n"
                                                                   "1: Warn on unused parameters\n"
                                                                   "2: Throw on unused parameters");

    if ((m_verbosity < 0) || (m_verbosity > 3)) throw JException("jana:parameter_verbosity: Bad parameter value!");
    if ((m_strictness < 0) || (m_strictness > 2)) throw JException("jana:parameter_strictness: Bad parameter value!");

    PrintParameters(m_verbosity, m_strictness);
}


/// @brief Prints parameters to stdout
///
/// @param [in] int verbosity   0: Don't show parameters table
///                             1: Show parameters with user-provided values only
///                             2: Show parameters with default values, except those marked "advanced"
///                             3: Show parameters with default values, including those marked "advanced"
///
/// @param [in] int strictness  0: Ignore unused parameters
///                             1: Warn on unused parameters
///                             2: Throw on unused parameters
void JParameterManager::PrintParameters(int verbosity, int strictness) {

    if (verbosity == 0) {
        LOG_INFO(m_logger) << "Configuration parameters table hidden. Set jana:parameter_verbosity > 0 to view." << LOG_END;
        return;
    }

    bool strictness_violation = false;

    // Unused parameters first 
    // The former might change the table columns and the latter might change the filter verbosity
    for (auto& pair : m_parameters) {
        const auto& key = pair.first;
        auto param = pair.second;
        
        if ((strictness > 0) && (!param->IsDefault()) && (!param->IsUsed())) {
            strictness_violation = true;
            LOG_ERROR(m_logger) << "Parameter '" << key << "' appears to be unused. Possible typo?" << LOG_END;
        }
        if ((!param->IsDefault()) && (param->IsDeprecated())) {
            LOG_ERROR(m_logger) << "Parameter '" << key << "' has been deprecated and may no longer be supported in future releases." << LOG_END;
        }
    }

    if (strictness_violation) {
        LOG_WARN(m_logger) << "Printing parameter table with full verbosity due to strictness warning" << LOG_END;
        verbosity = 3; // Crank up verbosity before printing out table
    }

    // Filter table
    vector<JParameter*> params_to_print;

    for (auto& pair : m_parameters) {
        auto param = pair.second;

        if (param->IsDeprecated() && (param->IsDefault())) continue;      
        // Always hide deprecated parameters that are NOT in use
        
        if ((verbosity == 1) && (param->IsDefault())) continue;           
        // At verbosity level 1, hide all default-valued parameters
        
        if ((verbosity == 2) && (param->IsDefault()) && (param->IsAdvanced())) continue;
        // At verbosity level 2, hide only advanced default-valued parameters
        
        params_to_print.push_back(param);
    } 

    // If all params are set to default values, then print a one line summary and return
    if (params_to_print.empty()) {
        LOG_INFO(m_logger) << "All configuration parameters set to default values." << LOG_END;
        return;
    }

    std::ostringstream ss;
    for (JParameter* p: params_to_print) {

        ss << std::endl;
        ss << " - key:         \"" << p->GetKey() << "\"" << std::endl;
        if (!p->IsDefault()) {
            ss << "   value:       \"" << p->GetValue() << "\"" << std::endl;
        }
        ss << "   default:     \"" << p->GetDefault() << "\"" << std::endl;
        if (!p->GetDescription().empty()) {
            ss << "   description: ";
            std::istringstream iss(p->GetDescription());
            std::string line;
            bool is_first_line =  true;
            while (std::getline(iss, line)) {
                if (!is_first_line) {
                    ss << std::endl << "                 " << line;
                }
                else {
                    ss << line;
                }
                is_first_line = false;
            }
            ss << std::endl;
        }
        if (p->IsConflicted()) {
            ss << "   warning:     Conflicting defaults" << std::endl;
        }
        if (p->IsDeprecated()) {
            ss << "   warning:     Deprecated" << std::endl;
            // If deprecated, it no longer matters whether it is advanced or not. If unused, won't show up here anyway.
        }
        if (!p->IsUsed()) {
            // Can't be both deprecated and unused, since JANA only finds out that it is deprecated by trying to use it
            // Can't be both advanced and unused, since JANA only finds out that it is advanced by trying to use it
            ss << "   warning:     Unused" << std::endl;
        }
        if (p->IsAdvanced()) {
            ss << "   warning:     Advanced" << std::endl;
        }
    }
    LOG_WARN(m_logger) << "Configuration Parameters\n"  << ss.str() << LOG_END;

    // Now that we've printed the table, we can throw an exception if we are being super strict
    if (strictness_violation && strictness > 1) {
        throw JException("Unrecognized parameters found! Throwing an exception because jana:parameter_strictness=2. See full parameters table in log.");
    }
}

/// @brief Access entire map of parameters
///
/// \return The parameter map
///
/// @details Use this to do things like writing all parameters out to a non-standard format.
/// This creates a copy of the map. Any modifications you make to the map itself won't propagate
/// back to the JParameterManager. However, any modifications you make to the enclosed JParameters
/// will. Prefer using SetParameter, SetDefaultParameter, FindParameter, or FilterParameters instead.
std::map<std::string, JParameter*> JParameterManager::GetAllParameters() {
    return m_parameters;
}

/// @brief Load parameters from a configuration file
///
/// @param filename    Path to the configuration file
///
/// @details
/// The file should have the form:
///     <pre>
///     key1 value1
///     key2 value2
///     ...
///     </pre>
///
/// There should be a space between the key and the value. The key may contain no spaces.
/// The value is taken as the rest of the line up to, but not including the newline.
/// A key may be specified with no value and the value will be set to "1".
/// A "#" charater will discard the remaining characters in a line up to
/// the next newline. Lines starting with "#" are ignored completely.
/// Lines with no characters (except for the newline) are ignored.
///
void JParameterManager::ReadConfigFile(std::string filename) {

    // Try and open file
    ifstream ifs(filename);

    if (!ifs.is_open()) {
        LOG_ERROR(m_logger) << "Unable to open configuration file \"" << filename << "\" !" << LOG_END;
        throw JException("Unable to open configuration file");
    }

    // Loop over lines
    char line[8192];
    while (!ifs.eof()) {
        // Read in next line ignoring comments
        bzero(line, 8192);
        ifs.getline(line, 8190);
        if (strlen(line) == 0) continue;
        if (line[0] == '#') continue;
        string str(line);

        // Check for comment character and erase comment if found
        size_t comment_pos = str.find('#');
        if (comment_pos != string::npos) {

            // Parameter descriptions are automatically added to configuration dumps
            // by adding a space, then the '#'. For string parameters, this extra
            // space shouldn't be there so check for it and delete it as well if found.
            if (comment_pos > 0 && str[comment_pos - 1] == ' ')comment_pos--;

            str.erase(comment_pos);
        }

        // Break line into tokens
        vector<string> tokens;
        string buf; // Have a buffer string
        stringstream ss(str); // Insert the string into a stream
        while (ss >> buf)tokens.push_back(buf);
        if (tokens.size() < 1)continue; // ignore empty lines

        // Use first token as key
        string key = tokens[0];

        // Concatenate remaining tokens into val string
        string val = "";
        for (unsigned int i = 1; i < tokens.size(); i++) {
            if (i != 1)val += " ";
            val += tokens[i];
        }
        if (val == "")val = "1";

        // Override defaulted values only
        JParameter* param = FindParameter(key);
        if (param == nullptr || param->HasDefault()) {
            SetParameter(key, val);
        }
    }

    // close file
    ifs.close();
}


/// @brief Write parameters out to an ASCII configuration file
///
/// @param [in] filename    Path to the configuration file
///
/// @details The file is written in a format compatible with reading in via ReadConfigFile().
///
void JParameterManager::WriteConfigFile(std::string filename) {

    // Try and open file
    ofstream ofs(filename);
    if (!ofs.is_open()) {
        LOG_ERROR(m_logger) << "Unable to open configuration file \"" << filename << "\" for writing!" << LOG_END;
        throw JException("Unable to open configuration file for writing!");
    }

    // Write header
    time_t t = time(NULL);
    string timestr(ctime(&t));
    ofs << "#" << endl;
    ofs << "# JANA configuration parameters" << endl;
    ofs << "# Created " << timestr;
    ofs << "#" << endl;
    ofs << endl;

    // Lock mutex to prevent manipulation of parameters while we're writing
    //_mutex.lock();

    // First, find the longest key and value
    size_t max_key_len = 0;
    size_t max_val_len = 0;
    for (auto pair : m_parameters) {
        auto key_len = pair.first.length();
        auto val_len = pair.second->GetValue().length();
        if (key_len > max_key_len) max_key_len = key_len;
        if (val_len > max_val_len) max_val_len = val_len;
    }

    // Loop over parameters a second time and print them out
    for (auto pair : m_parameters) {
        std::string key = pair.first;
        std::string val = pair.second->GetValue();
        std::string desc = pair.second->GetDescription();

        ofs << key << string(max_key_len - key.length(), ' ') << " "
            << val << string(max_val_len - val.length(), ' ') << " # "
            << desc << std::endl;
    }
    // Release mutex
    //_mutex.unlock();

    ofs.close();
}


/// @brief Copy a list of all parameters into the supplied map, replacing its current contents
///
/// @param [in] filter        Empty by default. If the filter string is non-empty, it is used to filter
///                           out all parameters whose key does not start with the filter string.
///                           Furthermore, the filter string is removed from the keys.
///
/// @param [out] parms        A map of {key: string, parameter: string}. The contents of this
///                           map are replaced on each call. Parameter values are returned
///                           as strings rather than as JParameter objects, which means we don't
///                           have to worry about ownership. Returned parameters are marked as used.
///
void JParameterManager::FilterParameters(std::map<std::string, std::string> &parms, std::string filter) {

    parms.clear();
    std::lock_guard<std::mutex> lock(m_mutex);
    for (auto pair : m_parameters) {
        string key = pair.second->GetKey();  // Note that this is the version that preserves the original case!
        string value = pair.second->GetValue();
        if (filter.size() > 0) {
            if (key.substr(0, filter.size()) != filter) continue;
            key.erase(0, filter.size());
        }
        pair.second->SetIsUsed(true);
        parms[key] = value;
    }
}

JLogger JParameterManager::GetLogger(const std::string& component_prefix) {

    JLogger logger;
    logger.group = component_prefix;

    auto global_log_level = RegisterParameter("jana:global_loglevel", JLogger::Level::INFO, "Global log level");

    bool enable_timestamp = RegisterParameter("jana:log:show_timestamp", true, "Show timestamp in log output");
    auto enable_threadstamp = RegisterParameter("jana:log:show_threadstamp", false, "Show threadstamp in log output");
    auto enable_group = RegisterParameter("jana:log:show_group", false, "Show threadstamp in log output");
    auto enable_level = RegisterParameter("jana:log:show_level", true, "Show threadstamp in log output");

    if (component_prefix.empty()) {
        logger.level = global_log_level;
    }
    else {
        std::ostringstream os;
        os << component_prefix << ":loglevel";
        logger.level = RegisterParameter(os.str(), global_log_level, "Component log level");
    }
    logger.ShowLevel(enable_level);
    logger.ShowTimestamp(enable_timestamp);
    logger.ShowThreadstamp(enable_threadstamp);
    logger.ShowGroup(enable_group);
    logger.show_threadstamp = enable_threadstamp;
    return logger;
}


