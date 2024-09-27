
// Copyright 2020, Jefferson Science Associates, LLC.
// Subject to the terms in the LICENSE file found in the top-level directory.

#pragma once

#include <JANA/Compatibility/JStreamLog.h>

#include <atomic>
#include <iostream>
#include <sstream>
#include <chrono>
#include <thread>
#include <iomanip>



struct JLogger {
    enum class Level { TRACE, DEBUG, INFO, WARN, ERROR, FATAL, OFF };
    Level level;
    std::ostream *destination;
    std::string className;
    bool show_level = true;
    bool show_classname = false;
    bool show_timestamp = true;
    bool show_threadstamp = false;

    explicit JLogger(JLogger::Level level = JLogger::Level::INFO,
                     std::ostream* destination = &std::cout,
                     std::string className = "")
            : level(level), destination(destination), className(std::move(className)) {};

    JLogger(const JLogger&) = default;
    JLogger& operator=(const JLogger&) = default;

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
        case JLogger::Level::TRACE: return s << "trace";
        case JLogger::Level::DEBUG: return s << "debug";
        case JLogger::Level::INFO:  return s << "info";
        case JLogger::Level::WARN:  return s << "warn ";
        case JLogger::Level::ERROR: return s << "error";
        case JLogger::Level::FATAL: return s << "fatal";
        default:               return s << "off";
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

        if (logger.show_timestamp) {
            auto now = std::chrono::system_clock::now();
            std::time_t current_time = std::chrono::system_clock::to_time_t(now);
            std::tm* local_time = std::localtime(&current_time);
            char buffer[100];
            std::strftime(buffer, sizeof(buffer), "%H:%M:%S", local_time);

            // Extract milliseconds by calculating the duration since the last whole second
            auto milliseconds = std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch()) % 1000;
            builder << buffer << ".";
            builder << std::setfill('0') << std::setw(5) << milliseconds << std::setfill(' ') << " ";
        }
        if (logger.show_level) {
            switch (level) {
                case JLogger::Level::TRACE: builder << "[trace] "; break;
                case JLogger::Level::DEBUG: builder << "[debug] "; break;
                case JLogger::Level::INFO:  builder << "[info]  "; break;
                case JLogger::Level::WARN:  builder << "[warn]  "; break;
                case JLogger::Level::ERROR: builder << "[error] "; break;
                case JLogger::Level::FATAL: builder << "[fatal] "; break;
                default: builder << "[?????] ";
            }
        }
        if (logger.show_threadstamp) {
            builder << "{" << std::this_thread::get_id() << "} ";
        }
        if (logger.show_classname) {
            builder << logger.className << ": ";
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


/// Stream operators

template <typename T>
inline JLogMessage operator<<(JLogger& l, const T& t) {
    JLogMessage m(l);
    m.builder << t;
    return m;
}

template<typename T>
inline JLogMessage& operator<<(JLogMessage& m, const T& t) {
    m.builder << t;
    return m;
}

template<typename T>
inline JLogMessage&& operator<<(JLogMessage&& m, const T& t) {
    m.builder << t;
    return std::move(m);
}

inline void operator<<(JLogMessage&& m, JLogMessage::End const&) {
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

