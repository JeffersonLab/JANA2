//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
// Jefferson Science Associates LLC Copyright Notice:
//
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
// Author: Nathan Brei
//

#ifndef JANA2_JSERVICELOCATORTESTS_H
#define JANA2_JSERVICELOCATORTESTS_H
#pragma once

#include <map>
#include <JANA/Services/JServiceLocator.h>

struct ParameterSvc : public JService {

    std::map<std::string, std::string> underlying;

    void acquire_services(JServiceLocator *sl) {}
    void set(std::string name, std::string value) {
        underlying[name] = value;

    }
    std::string get(std::string name) {
        return underlying[name];
    }
};


struct LoggerSvc : public JService {

    std::string level = "INFO";

    void acquire_services(JServiceLocator *sl) {
        level = sl->get<ParameterSvc>()->get("JANA:LogLevel");
    }

};

struct MagneticFieldSvc : public JService {

    std::shared_ptr<LoggerSvc> logger;
    std::string url = "NO_URL";

    void acquire_services(JServiceLocator *sl) {
        logger = sl->get<LoggerSvc>();
        url = sl->get<ParameterSvc>()->get("EIC:MagneticFieldDatabaseURL");
        if (url == "") {
            sl->get<ParameterSvc>()->set("EIC:MagneticFieldDatabaseURL", "mysql://127.0.0.1");
        } // In reality these are both one function call that registers the param if it doesn't exist,
        // sets a default value, and returns a value that the user may have specified and the default
        // value if the user didn't
    }

    bool connect() {
        return url != "NO_URL";
    }
};

#endif //JANA2_JSERVICELOCATORTESTS_H
