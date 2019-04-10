

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


enum class JLogLevel { TRACE, DEBUG, INFO, WARN, ERROR, FATAL, OFF };

inline std::ostream& operator<<(std::ostream& s, JLogLevel l) {
	switch (l) {
		case JLogLevel::TRACE: return s << "TRACE";
		case JLogLevel::DEBUG: return s << "DEBUG";
		case JLogLevel::INFO:  return s << "INFO";
		case JLogLevel::WARN:  return s << "WARN";
		case JLogLevel::ERROR: return s << "ERROR";
		case JLogLevel::FATAL: return s << "FATAL";
		case JLogLevel::OFF:   return s << "OFF";
	}
	return s << "UNKNOWN";
}


struct JLogger {
	JLogLevel level = JLogLevel::INFO;
	std::ostream& destination = std::cout;
	std::mutex mutex;
};

struct JLogMessageEnd {};

struct JLogMessage {

	std::shared_ptr<JLogger> logger;
	JLogLevel level;
	std::ostringstream builder;


  JLogMessage(int throwaway = 0) : level(JLogLevel::DEBUG) {}

	JLogMessage(std::shared_ptr<JLogger> logger_, JLogLevel level_) :
		logger(logger_), level(level_) {

		builder << "[" << level << "] ";
	}

	// Helper function for truncating long strings to keep our log readable
	// This should probably live somewhere else
	std::string ltrunc(std::string original, size_t desired_length) {
		auto n = original.length();
		if (n <= desired_length) return original;
		return "\u2026" + original.substr(n-desired_length, desired_length-1);
	}


	JLogMessage(std::shared_ptr<JLogger> logger_,
		    JLogLevel level_,
		    int thread,
		    std::string file,
		    int line,
		    std::string func,
		    long timestamp
		) : logger(logger_), level(level_) {

		builder << std::setw(5) << std::left << level << " "
	     		<< "[" << std::setw(2) << std::right << thread << "] "
	     		<< std::setw(20) << std::right << ltrunc(file,20) << ":"
	     		<< std::setw(5) << std::left << line << " "
	     		<< std::setw(12) << std::left << ltrunc(func,12) << " ";
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

inline void operator<<(JLogMessage && m, JLogMessageEnd const & end) {
	std::lock_guard<std::mutex> lock(m.logger->mutex);
  if (m.logger == nullptr) {
    std::cout << m.builder.str() << std::endl;
  } else {
    m.logger->destination << m.builder.str() << std::endl;
  }
}


#define VLOG(logger, msglevel) if (logger->level <= msglevel) JLogMessage(logger, msglevel, THREAD_ID, __FILE__, __LINE__, __func__, 0)

#define VLOG_FATAL(logger) VLOG(logger, JLogLevel::FATAL)
#define VLOG_ERROR(logger) VLOG(logger, JLogLevel::ERROR)
#define VLOG_WARN(logger)  VLOG(logger, JLogLevel::WARN)
#define VLOG_INFO(logger)  VLOG(logger, JLogLevel::INFO)
#define VLOG_DEBUG(logger) VLOG(logger, JLogLevel::DEBUG)
#define VLOG_TRACE(logger) VLOG(logger, JLogLevel::TRACE)


#define LOG(logger, msglevel) if (logger->level <= msglevel) JLogMessage(logger, msglevel)

#define LOG_FATAL(logger) LOG(logger, JLogLevel::FATAL)
#define LOG_ERROR(logger) LOG(logger, JLogLevel::ERROR)
#define LOG_WARN(logger)  LOG(logger, JLogLevel::WARN)
#define LOG_INFO(logger)  LOG(logger, JLogLevel::INFO)
#define LOG_DEBUG(logger) LOG(logger, JLogLevel::DEBUG)
#define LOG_TRACE(logger, flag) if (logger->level <= JLogLevel::TRACE || flag) JLogMessage(logger, JLogLevel::TRACE)

#define LOG_END JLogMessageEnd()



// Logging for when you don't have a JLogger readily available
// The interface here is chosen to be backward-compatible with JLog for now

typedef JLogMessage JLog;
struct JLogEnd {};

inline void operator<<(JLogMessage && m, JLogEnd const & end) {
  std::cout << m.builder.str();
}

inline void operator<<(JLogMessage & m, JLogEnd const & end) {
  std::cout << m.builder.str();
}

#endif



