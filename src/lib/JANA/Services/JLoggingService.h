

#ifndef JANA_JLOGGER_H_
#define JANA_JLOGGER_H_

#include <JANA/JLogger.h>
#include <JANA/Services/JParameterManager.h>
#include <JANA/Services/JServiceLocator.h>

#include <iostream>
#include <sstream>
#include <mutex>
#include <memory>
#include <iomanip>

// Convenience macros for temporary debugging statements.
#ifndef _DBG__
#define _DBG__ std::cerr<<__FILE__<<":"<<__LINE__<<std::endl
#define _DBG_  std::cerr<<__FILE__<<":"<<__LINE__<<" "
#endif // _DBG__


// Helper function for truncating long strings to keep our log readable
inline std::string ltrunc(std::string original, size_t desired_length) {
    auto n = original.length();
    if (n <= desired_length) return original;
    return "\u2026" + original.substr(n - desired_length, desired_length - 1);
}

class JLoggingService : public JService {

    JLogger::Level _global_log_level = JLogger::Level::INFO;
    std::map<std::string, JLogger::Level> _local_log_levels;

public:

    void set_level(JLogger::Level level) { _global_log_level = level; }

    void set_level(std::string className, JLogger::Level level) {
        _local_log_levels[className] = level;
    }

    void acquire_services(JServiceLocator* serviceLocator) override {

        auto params = serviceLocator->get<JParameterManager>();
        std::vector<std::string> groups;
        params->SetDefaultParameter("log:off", groups, "");
        for (auto& s : groups) {
            _local_log_levels[s] = JLogger::Level::OFF;
        }
        groups.clear();
        params->SetDefaultParameter("log:fatal", groups, "");
        for (auto& s : groups) {
            _local_log_levels[s] = JLogger::Level::FATAL;
        }
        groups.clear();
        params->SetDefaultParameter("log:error", groups, "");
        for (auto& s : groups) {
            _local_log_levels[s] = JLogger::Level::ERROR;
        }
        groups.clear();
        params->SetDefaultParameter("log:warn", groups, "");
        for (auto& s : groups) {
            _local_log_levels[s] = JLogger::Level::WARN;
        }
        groups.clear();
        params->SetDefaultParameter("log:info", groups, "");
        for (auto& s : groups) {
            _local_log_levels[s] = JLogger::Level::INFO;
        }
        groups.clear();
        params->SetDefaultParameter("log:debug", groups, "");
        for (auto& s : groups) {
            _local_log_levels[s] = JLogger::Level::DEBUG;
        }
        groups.clear();
        params->SetDefaultParameter("log:trace", groups, "");
        for (auto& s : groups) {
            _local_log_levels[s] = JLogger::Level::TRACE;
        }
    }

    JLogger get_logger() {
        return JLogger(_global_log_level);
    }

    JLogger get_logger(std::string className) {

        JLogger logger;
        auto search = _local_log_levels.find(className);
        if (search != _local_log_levels.end()) {
            logger.level = search->second;
        } else {
            logger.level = _global_log_level;
        }
        return logger;
    }

    /// This is a convenience function to get a correctly configured Logger via a ServiceLocator global.
    /// If we could use singletons, we wouldn't have to do things like this.
    static JLogger logger(std::string className) {

        if (serviceLocator != nullptr) {
            auto loggingService = serviceLocator->get<JLoggingService>();
            if (loggingService != nullptr) {
                return loggingService->get_logger(className);
            }
        }
        return JLogger(JLogger::Level::WARN);
    }
};


#endif



