

#ifndef JANA_JLOGNEW_H_
#define JANA_JLOGNEW_H_

#include <iostream>
#include <iomanip>
#include <mutex>


// JLogLevel provides 
enum class JLogLevel { TRACE, DEBUG, INFO, WARN, ERROR, FATAL, OFF };

ostream& operator<<(ostream& s, JLogLevel l) {
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



// JLog is a very thin abstraction over an output stream, adding information 
// such as log levels, threadID, etc. It is the responsibility of the caller to 
// provide an alternative output stream, which decouples JLog from JApplication, etc. 
// For the time being there is no encapsulation, although this may change 
// depending on human factors considerations.
struct JLogNew {
	
	JLogLevel level = JLogLevel::INFO;
	std::ostream& dest = std::cout;
	std::mutex mutex;

	// Helper function for truncating long strings to keep our log readable
	// This should probably live somewhere else
	string ltruncate(string original, size_t desired_length) {
		auto n = original.length();
		if (n <= desired_length) return original;
		return "\u2026" + original.substr(n-desired_length, desired_length-1);
	}

	// Append takes semistructured logging info and emits it into the ostream.
	// Use the macros below instead of calling this directly
	void append(JLogLevel messageLevel, int threadId, const string& filename, 
		    const string& funcname, const int line, const string& message) {

		if (messageLevel < level) return;

		std::lock_guard<std::mutex> lock(mutex);

		dest << setw(5) << std::left << messageLevel << " " 
		     << "[" << setw(2) << std::right << threadId << "] " 
		     << setw(12) << std::right << ltruncate(funcname,12) << " "
		     << setw(20) << std::right << ltruncate(filename,20) << ":"
		     << setw(5) << std::left << line
		     << message << std::endl;
	}
};

// Since logging cuts horizontally across the entire program, it makes 
// sense to maintain a bit of global state just like std::cout, in lieu of 
// introducing dependency injection. 
JLogNew global_logger;


// Logging introduces a significant overhead unless the string operations 
// are protected by an if-statement. Similarly, figuring out the current 
// threadID is very weird. We wish to insulate the user from these
// worries, so we direct them to use these macros instead of JLogNew::append()
//
// TODO: This still has a dependency on the JTHREAD global variable. Ideally
// we can make our logger independent of our threading internals, but that 
// will have to come later.
#define JLOG(msglevel, msgbody) \
	{ \
	JLogLevel temp = (msglevel); \
	if(global_logger.level <= temp) { \
		int threadId = (JTHREAD == nullptr) ? -1 : JTHREAD->GetThreadID(); \
		global_logger.append(temp, threadId, __FILE__, __func__, __LINE__, (msgbody)); \
	}} 

#define LOG_TRACE(message) JLOG(JLogLevel::TRACE, message)
#define LOG_DEBUG(message) JLOG(JLogLevel::DEBUG, message)
#define LOG_INFO(message)  JLOG(JLogLevel::INFO, message)
#define LOG_WARN(message)  JLOG(JLogLevel::WARN, message)
#define LOG_ERROR(message) JLOG(JLogLevel::ERROR, message)
#define LOG_FATAL(message) JLOG(JLogLevel::FATAL, message)

#endif



