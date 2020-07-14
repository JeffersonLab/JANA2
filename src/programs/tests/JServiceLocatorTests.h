
// Copyright 2020, Jefferson Science Associates, LLC.
// Subject to the terms in the LICENSE file found in the top-level directory.

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
