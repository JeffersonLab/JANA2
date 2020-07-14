
// Copyright 2020, Jefferson Science Associates, LLC.
// Subject to the terms in the LICENSE file found in the top-level directory.


#ifndef JANA2_JLOGGER_H
#define JANA2_JLOGGER_H

#include <iostream>
#include <sstream>

///
struct JLogger {
    enum class Level { TRACE, DEBUG, INFO, WARN, ERROR, FATAL, OFF };
    Level level;
    std::ostream *destination;
    std::string className;
    bool show_level = true;
    bool show_classname = false;
    bool show_timestamp = false;
    bool show_threadstamp = false;

    explicit JLogger(JLogger::Level level = JLogger::Level::INFO,
                     std::ostream* destination = &std::cout,
                     std::string className = "")
            : level(level), destination(destination), className(std::move(className)) {};

    JLogger(const JLogger& logger) = default;
    JLogger& operator=(const JLogger& in) = default;

    void SetTag(std::string tag) {className = tag; }
    void SetTimestampFlag() {show_timestamp = true; }
    void UnsetTimestampFlag() {show_timestamp = false; }
    void SetThreadstampFlag() {show_threadstamp = true; }
    void UnsetThreadstampFlag() {show_threadstamp = false; }
};

static JLogger default_cout_logger = JLogger(JLogger::Level::TRACE, &std::cout, "JANA");
static JLogger default_cerr_logger = JLogger(JLogger::Level::TRACE, &std::cerr, "JERR");

inline std::ostream& operator<<(std::ostream& s, JLogger::Level l) {
    switch (l) {
        case JLogger::Level::TRACE: return s << "TRACE";
        case JLogger::Level::DEBUG: return s << "DEBUG";
        case JLogger::Level::INFO:  return s << "INFO";
        case JLogger::Level::WARN:  return s << "WARN";
        case JLogger::Level::ERROR: return s << "ERROR";
        case JLogger::Level::FATAL: return s << "FATAL";
        default:               return s << "OFF";
    }
}

///
struct JLogMessage {

    /// Message terminator
    struct End {};

    const JLogger& logger;
    JLogger::Level level;
    std::ostringstream builder;

    JLogMessage(const JLogger& logger = default_cout_logger,
                JLogger::Level level = JLogger::Level::INFO)
                : logger(logger), level(level) {

        if (logger.show_level) {
            builder << "[" << level << "] ";
        }
        if (logger.show_classname) {
            builder << logger.className << ": ";
        }
        // TODO: Re-add thread and timestamp info?
    }

    // Helper function for truncating long strings to keep our log readable
    // This should probably live somewhere else
    std::string ltrunc(std::string original, size_t desired_length) {
        auto n = original.length();
        if (n <= desired_length) return original;
        return "\u2026" + original.substr(n - desired_length, desired_length - 1);
    }
};


/// Stream operators

template <typename T>
inline JLogMessage&& operator<<(JLogger& l, T t) {
    JLogMessage m(l);
    m.builder << t;
    return std::move(m);
}

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

inline void operator<<(JLogMessage&& m, JLogMessage::End const& end) {
    std::ostream& dest = *m.logger.destination;
    m.builder << std::endl;
    dest << m.builder.str();
    dest.flush();
}

/// Macros

#define LOG JLogMessage()

#define LOG_IF(predicate) if (predicate) JLogMessage()

#define LOG_END JLogMessage::End();

#define LOG_AT_LEVEL(logger, msglevel) if ((logger).level <= msglevel) JLogMessage((logger), msglevel)

#define LOG_FATAL(logger) LOG_AT_LEVEL(logger, JLogger::Level::FATAL)
#define LOG_ERROR(logger) LOG_AT_LEVEL(logger, JLogger::Level::ERROR)
#define LOG_WARN(logger)  LOG_AT_LEVEL(logger, JLogger::Level::WARN)
#define LOG_INFO(logger)  LOG_AT_LEVEL(logger, JLogger::Level::INFO)
#define LOG_DEBUG(logger) LOG_AT_LEVEL(logger, JLogger::Level::DEBUG)
#define LOG_TRACE(logger) LOG_AT_LEVEL(logger, JLogger::Level::TRACE)


#define jout JLogMessage(default_cout_logger)
#define jerr JLogMessage(default_cerr_logger)
#define jendl JLogMessage::End()



#endif //JANA2_JLOGGER_H
