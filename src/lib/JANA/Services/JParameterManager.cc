//
//    File: JParameterManager.cc
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

#include "JParameterManager.h"
#include "JLoggingService.h"

#include <vector>
#include <string>
#include <fstream>
#include <cstring>

using namespace std;


//---------------------------------
// JParameterManager    (Constructor)
//---------------------------------
JParameterManager::JParameterManager() {
    _logger = JLoggingService::logger("JParameterManager");
}

//---------------------------------
// ~JParameterManager    (Destructor)
//---------------------------------
JParameterManager::~JParameterManager() {
    for (auto p : _jparameters) delete p.second;
    _jparameters.clear();
}

//---------------------------------
// Exists
//---------------------------------
bool JParameterManager::Exists(string name) {
    return _jparameters.count(ToLC(name)) != 0;
}

//---------------------------------
// FindParameter
//---------------------------------
JParameter* JParameterManager::FindParameter(std::string name) {
    if (!Exists(name)) return nullptr;

    return _jparameters[ToLC(name)];
}

//---------------------------------
// PrintParameters
//---------------------------------
void JParameterManager::PrintParameters(bool all) {
    /// Print configuration parameters to stdout.
    /// If "all" is false (default) then only parameters
    /// whose values are different than their default are
    /// printed.
    /// If "all" is true then all parameters are
    /// printed.

    // Find maximum key length
    uint32_t max_key_len = 4;
    vector<string> keys;
    for (auto& p : _jparameters) {
        string key = p.first;
        auto j = p.second;
        if ((!all) && j->IsDefault()) continue;
        keys.push_back(key);
        if (key.length() > max_key_len) max_key_len = key.length();
    }

    // If all params are set to default values, then print a one line
    // summary
    if (keys.empty()) {
        LOG << "All configuration parameters set to default values." << LOG_END;
        return;
    }

    // Print title/header
    JLogMessage m;
    string title("Config. Parameters");
    uint32_t half_title_len = 1 + title.length() / 2;
    if (max_key_len < half_title_len) max_key_len = half_title_len;
    m << "Parameters\n\n"
        << string(max_key_len + 4 - half_title_len, ' ') << title << "\n"
        << "  " << string(2 * max_key_len + 3, '=') << "\n"
        << string(max_key_len / 2, ' ') << "name" << string(max_key_len, ' ') << "value" << "\n"
        << "  " << string(max_key_len, '-') << "   " << string(max_key_len, '-') << "\n";

    // Print all parameters
    for (string& key : keys) {
        auto name = _jparameters[key]->GetName();
        string val = _jparameters[key]->GetValue<string>();
        m << string(max_key_len + 2 - key.length(), ' ') << name << " = " << val << "\n";
    }
    std::move(m) << LOG_END;
}


void JParameterManager::ReadConfigFile(std::string filename) {
    /// Read in the configuration file with name specified by "fname".
    /// The file should have the form:
    ///
    /// <pre>
    /// key1 value1
    /// key2 value2
    /// ...
    /// </pre>
    ///
    /// Where there is a space between the key and the value (thus, the "key"
    /// can contain no spaces). The value is taken as the rest of the line
    /// up to, but not including the newline itself.
    ///
    /// A key may be specified with no value and the value will be set to "1".
    ///
    /// A "#" charater will discard the remaining characters in a line up to
    /// the next newline. Therefore, lines starting with "#" are ignored
    /// completely.
    ///
    /// Lines with no characters (except for the newline) are ignored.

    // Try and open file
    ifstream ifs(filename);

    if (!ifs.is_open()) {
        LOG_ERROR(_logger) << "Unable to open configuration file \"" << filename << "\" !" << LOG_END;
        throw JException("Unable to open configuration file");
    }

    // Loop over lines
    char line[1024];
    while (!ifs.eof()) {
        // Read in next line ignoring comments
        ifs.getline(line, 1024);
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
        if (!Exists(key) || FindParameter(key)->IsDefault()) {
            SetParameter(key, val);
        }
    }

    // close file
    ifs.close();
}

//---------------------------------
// WriteConfigFile
//---------------------------------
void JParameterManager::WriteConfigFile(std::string filename) {
    /// Write all of the configuration parameters out to an ASCII file in
    /// a format compatible with reading in via ReadConfigFile().

    // Try and open file
    ofstream ofs(filename);
    if (!ofs.is_open()) {
        LOG_ERROR(_logger) << "Unable to open configuration file \"" << filename << "\" for writing!" << LOG_END;
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
    for (auto pair : _jparameters) {
        auto key_len = pair.first.length();
        auto val_len = pair.second->GetValue<std::string>().length();
        if (key_len > max_key_len) max_key_len = key_len;
        if (val_len > max_val_len) max_val_len = val_len;
    }

    // Loop over parameters a second time and print them out
    for (auto pair : _jparameters) {
        std::string key = pair.first;
        std::string val = pair.second->GetValue<std::string>();

        ofs << key << string(max_key_len - key.length(), ' ') << " " << val << string(max_val_len - val.length(), ' ');
        // TODO: No ability to retrieve descriptions!
        ofs << std::endl;
    }
    // Release mutex
    //_mutex.unlock();

    // close file
    ofs.close();
}
