//*********************************************************************
// JLog.cc - Implementation file for the JLog class. See the dot H file
// for the implementation of the "log" method.
// Author: Craig Bookwalter (craigb@jlab.org)
// July 2005
//********************************************************************

#include "JLog.h"
#include <stdexcept>
#include <time.h>

JLog::JLog() {
	__info  = &std::cout;
	__warn  = &std::cout;
	__err   = &std::cerr;
	__level = ERR;
}

JLog::JLog(std::ostream& info, 
		   std::ostream& warn, 
		   std::ostream& err, 
		   int level) 
{
	__info  = &info;
	__warn  = &warn;
	__err   = &err;
	__level = level;
}

JLog::~JLog() {}

void JLog::setLoggingLevel(int level) {
	__level = level;
}
