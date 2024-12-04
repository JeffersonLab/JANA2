
// Copyright 2020, Jefferson Science Associates, LLC.
// Subject to the terms in the LICENSE file found in the top-level directory.

#pragma once

#include <JANA/Compatibility/JStreamLog.h>

#include <iostream>
#include <sstream>
#include <chrono>
#include <thread>
#include <iomanip>
#include <time.h>


struct JLogger {
    enum class Level { TRACE, DEBUG, INFO, WARN, ERROR, FATAL, OFF };
    Level level;
    std::ostream *destination;
    std::string group;
    bool show_level = true;
    bool show_group = false;
    bool show_timestamp = true;
    bool show_threadstamp = false;

    explicit JLogger(JLogger::Level level = JLogger::Level::INFO,
                     std::ostream* destination = &std::cout,
                     std::string group = "")
            : level(level), destination(destination), group(std::move(group)) {};

    JLogger(const JLogger&) = default;
    JLogger& operator=(const JLogger&) = default;

    void SetGroup(std::string group) {this->group = group; }
    void ShowGroup(bool show) {show_group = show; }
    void ShowLevel(bool show) {show_level = show; }
    void ShowTimestamp(bool show) {show_timestamp = show; }
    void ShowThreadstamp(bool show) {show_threadstamp = show; }
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


class JLogMessage : public std::stringstream {
public:
    struct Flush {};

private:
    std::string m_prefix;

public:
    JLogMessage(const std::string& prefix="") : m_prefix(prefix){
    }

    JLogMessage(const JLogger& logger, JLogger::Level level) {
        std::ostringstream builder;
        if (logger.show_timestamp) {
            auto now = std::chrono::system_clock::now();
            std::time_t current_time = std::chrono::system_clock::to_time_t(now);
            tm tm_buf;
            localtime_r(&current_time, &tm_buf);

            // Extract milliseconds by calculating the duration since the last whole second
            auto milliseconds = std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch()) % 1000;
            builder << std::put_time(&tm_buf, "%H:%M:%S.");
            builder << std::setfill('0') << std::setw(3) << milliseconds.count() << std::setfill(' ') << " ";
        }
        if (logger.show_level) {
            switch (level) {
                case JLogger::Level::TRACE: builder << "[trace] "; break;
                case JLogger::Level::DEBUG: builder << "[debug] "; break;
                case JLogger::Level::INFO:  builder << " [info] "; break;
                case JLogger::Level::WARN:  builder << " [warn] "; break;
                case JLogger::Level::ERROR: builder << "[error] "; break;
                case JLogger::Level::FATAL: builder << "[fatal] "; break;
                default: builder << "[?????] ";
            }
        }
        if (logger.show_threadstamp) {
            builder << std::this_thread::get_id() << " ";
        }
        if (logger.show_group && !logger.group.empty()) {
            builder << logger.group << " > ";
        }
        m_prefix = builder.str();
    }

    void do_flush() {
        std::string line;
        std::ostringstream oss;
        if (this->str().empty()) {
            // This way, `LOG_INFO(GetLogger()) << LOG_END;` produces a blank line
            oss << m_prefix << std::endl;
        }
        else {
            while (std::getline(*this, line)) {
                oss << m_prefix << line << std::endl;
            }
        }
        std::cout << oss.str();
        std::cout.flush();
        this->str("");
        this->clear();
    }

    virtual ~JLogMessage() {
        if (!this->str().empty()) {
            do_flush();
        }
    }
};


/// Stream operators

inline JLogMessage& operator<<(JLogMessage& msg, const JLogMessage::Flush&) {
    // This lets us force a flush
    msg.do_flush();
    return msg;
}

inline std::ostream& operator<<(std::ostream& msg, const JLogMessage::Flush&) {
    // This lets us force a flush
    auto* logmsg = dynamic_cast<JLogMessage*>(&msg);
    if (logmsg != nullptr) {
        logmsg->do_flush();
    }
    return msg;
}

/// Macros

#define LOG JLogMessage()

#define LOG_IF(predicate) if (predicate) JLogMessage()

#define LOG_END JLogMessage::Flush()

#define LOG_AT_LEVEL(logger, msglevel) if ((logger).level <= msglevel) JLogMessage((logger), msglevel)

#define LOG_FATAL(logger) LOG_AT_LEVEL(logger, JLogger::Level::FATAL)
#define LOG_ERROR(logger) LOG_AT_LEVEL(logger, JLogger::Level::ERROR)
#define LOG_WARN(logger)  LOG_AT_LEVEL(logger, JLogger::Level::WARN)
#define LOG_INFO(logger)  LOG_AT_LEVEL(logger, JLogger::Level::INFO)
#define LOG_DEBUG(logger) LOG_AT_LEVEL(logger, JLogger::Level::DEBUG)
#define LOG_TRACE(logger) LOG_AT_LEVEL(logger, JLogger::Level::TRACE)

