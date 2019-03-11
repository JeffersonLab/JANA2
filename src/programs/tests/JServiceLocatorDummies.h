#pragma once

#include <map>
#include <greenfield/ServiceLocator.h>
namespace greenfield {

    struct ParameterSvc : public Service {

        std::map<std::string, std::string> underlying;

        void acquire_services(ServiceLocator *sl) {}
        void set(std::string name, std::string value) {
            underlying[name] = value;

        }
        std::string get(std::string name) {
            return underlying[name];
        }
    };


    struct LoggerSvc : public Service {

        std::string level = "INFO";

        void acquire_services(ServiceLocator *sl) {
            level = sl->get<ParameterSvc>()->get("JANA:LogLevel");
        }

    };

    struct MagneticFieldSvc : public Service {

        LoggerSvc* logger;
        std::string url = "NO_URL";

        void acquire_services(ServiceLocator *sl) {
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
}
