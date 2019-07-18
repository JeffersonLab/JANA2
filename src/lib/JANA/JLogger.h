

#ifndef JANA_JLOGGER_H_
#define JANA_JLOGGER_H_

#include <iostream>
#include <sstream>
#include <mutex>
#include <memory>
#include <iomanip>

#include <cxxabi.h>
// This is here to prevent the compiler from getting confused
// while trying to instantiate operator<< templates.
// It has to be included _before_ operator<<(JLog&&, T) gets defined

#include <JANA/JServiceLocator.h>

enum class JLogLevel { TRACE, DEBUG, INFO, WARN, ERROR, FATAL, OFF };

inline std::ostream& operator<<(std::ostream& s, JLogLevel l) {
    switch (l) {
        case JLogLevel::TRACE: return s << "TRACE";
        case JLogLevel::DEBUG: return s << "DEBUG";
        case JLogLevel::INFO:  return s << "INFO";
        case JLogLevel::WARN:  return s << "WARN";
        case JLogLevel::ERROR: return s << "ERROR";
        case JLogLevel::FATAL: return s << "FATAL";
        default:               return s << "OFF";
    }
}

// Convenience macros for temporary debugging statements.
#ifndef _DBG__
#define _DBG__ std::cerr<<__FILE__<<":"<<__LINE__<<std::endl
#define _DBG_  std::cerr<<__FILE__<<":"<<__LINE__<<" "
#endif // _DBG__

struct JLogger {
    JLogLevel level;
    std::ostream* destination;
    std::string className;

    JLogger(JLogLevel level = JLogLevel::OFF, std::ostream* dest = &std::cout, std::string className = "")
            : level(level), destination(dest), className(std::move(className)) {};

    JLogger(const JLogger& logger) : level(logger.level), destination(logger.destination), className(logger.className) {}

    JLogger& operator=(const JLogger& in) {
        level = in.level;
        destination = in.destination;
        return *this;
    }

    static JLogger everything() {
        return JLogger(JLogLevel::TRACE);
    }

    static JLogger nothing() {
        return JLogger(JLogLevel::OFF);
    }
};

struct JLogMessageEnd {
};

extern thread_local int THREAD_ID;

struct JLogMessage {

    const JLogger* logger;
    JLogLevel level;
    std::ostringstream builder;

    JLogMessage(int throwaway = 0) : level(JLogLevel::DEBUG) {}

    JLogMessage(const JLogger* logger, JLogLevel level) :
            logger(logger), level(level) {
        if (logger->className != "") {
            builder << "[" << level << "] " << THREAD_ID << ": ";
        } else {
            builder << "[" << level << "] " << THREAD_ID << ": " << logger->className << ": ";
        }

    }

    // Helper function for truncating long strings to keep our log readable
    // This should probably live somewhere else
    std::string ltrunc(std::string original, size_t desired_length) {
        auto n = original.length();
        if (n <= desired_length) return original;
        return "\u2026" + original.substr(n - desired_length, desired_length - 1);
    }
};

template<typename T>
inline JLogMessage& operator<<(JLogMessage& m, T t) {
    m.builder << t;
    return m;
}

template<typename T>
inline JLogMessage&& operator<<(JLogMessage&& m, T t) {
    m.builder << t;
    return std::move(m);
}

inline void operator<<(JLogMessage&& m, JLogMessageEnd const& end) {
    if (m.logger == nullptr) {
        std::cout << m.builder.str() << std::endl;
    } else {
        (*m.logger->destination) << m.builder.str() << std::endl;
    }
}


#define LOG(logger, msglevel) if (logger.level <= msglevel) JLogMessage(&logger, msglevel)

#define LOG_FATAL(logger) LOG(logger, JLogLevel::FATAL)
#define LOG_ERROR(logger) LOG(logger, JLogLevel::ERROR)
#define LOG_WARN(logger)  LOG(logger, JLogLevel::WARN)
#define LOG_INFO(logger)  LOG(logger, JLogLevel::INFO)
#define LOG_DEBUG(logger) LOG(logger, JLogLevel::DEBUG)
#define LOG_TRACE(logger, flag) if (logger.level <= JLogLevel::TRACE || flag) JLogMessage(&logger, JLogLevel::TRACE)

#define LOG_END JLogMessageEnd()



// Logging for when you don't have a JLogger readily available
// The interface here is chosen to be backward-compatible with JLog for now

typedef JLogMessage JLog;

struct JLogEnd {};

inline void operator<<(JLogMessage&& m, JLogEnd const& end) {
    std::cout << m.builder.str();
}

inline void operator<<(JLogMessage& m, JLogEnd const& end) {
    std::cout << m.builder.str();
}


class JLoggingService : public JService {

    JLogLevel _global_log_level = JLogLevel::INFO;
    std::map<std::string, JLogLevel> _local_log_levels;

public:
    void set_level(JLogLevel level) { _global_log_level = level; }

    void set_level(std::string className, JLogLevel level) {
        _local_log_levels[className] = level;
    }

    void acquire_services(JServiceLocator* serviceLocator) override {
        // serviceLocator.get<ParameterService>()
        // Add loglevels to
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
        return JLogger(JLogLevel::WARN);
    }
};


#endif



