
#include "JStreamLog.h"

JStreamLog::JStreamLog(std::streambuf* buf, const char* tag) :
std::ostream(new JStreamLogBuffer(buf, tag))
{}

JStreamLog::JStreamLog(const std::ostream& os, const char* tag) :
std::ostream(new JStreamLogBuffer(os.rdbuf(), tag))
{}

JStreamLog::JStreamLog(const std::ostream& os, const std::string& tag) :
std::ostream(new JStreamLogBuffer(os.rdbuf(), tag.c_str()))
{}

JStreamLog::~JStreamLog() {
	delete rdbuf();
}

std::ostream& endMsg(std::ostream& dSL) {
	dSL << static_cast<char>(6); 
	dSL << std::flush;
	return dSL;
}
