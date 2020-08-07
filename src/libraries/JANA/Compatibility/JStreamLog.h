//*************************************************************
// JStreamLog.h - Header file for stream-based logging.
// Author:	Craig Bookwalter
// Date:	Aug 2005
// Notes: 	Much of this was derived from examples given by
// 	D. Kuehl at http://www.inf.uni-konstanz.de/~kuehl/iostream/
// 	Also, many thanks to J. Hardie for his assistance with 
//  obscure protected-member rules. 
//*************************************************************


#ifndef _DSTREAMLOG_H_
#define _DSTREAMLOG_H_

#include <iostream>
#include <fstream>
#include <string>
#include "JStreamLogBuffer.h"

/// JStreamLog provides an interface for for writing messages
/// in a way that buffers them by thread to prevent multiple
/// threads from simultaneously writing to the screen (via
/// cout and cerr) causing the characters to intermix resulting
/// in gibberish.
///
/// JANA using the jout and jerr global streams by default which
/// send their output to cout and cerr respectively. In addition,
/// they can optionally prepend a special "tag" string to indicate
/// which stream the message came from. By default, these are 
/// set to "JANA >>" and "JANA ERROR>>", but are user configureable.
/// One can also turn on prepending of timestamps to be printed
/// with each line (this is off by default).
///
/// JStreamLog objects inherit from std::ostream and so can be used
/// just like cout and cerr.
///
/// Example:
///
///    jout<<"Hello World!"<<endl;
///

class JStreamLog : public std::ostream
{
	public:
		JStreamLog(const std::ostream& os=std::cout, const char* tag="INFO");
		JStreamLog(const std::ostream& os, const std::string& tag);
		JStreamLog(std::streambuf* buf, const char* tag);
		virtual ~JStreamLog();
		
		std::string GetTag(void);
		bool GetTimestampFlag(void);
		bool GetThreadstampFlag(void);
		JStreamLogBuffer* GetJStreamLogBuffer(void);
		void SetTag(std::string tag);
		void SetTimestampFlag(bool prepend_timestamp=true);
		void SetThreadstampFlag(bool prepend_threadstamp=true);

	private:
		bool own_rdbuf; // keep track if we deleted the buffer object already
};

std::ostream& endMsg(std::ostream& os);

extern JStreamLog jout;
extern JStreamLog jerr;

#endif //_DSTREAMLOG_H_
