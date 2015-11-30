//************************************************************
// JStreamLogBuffer.h - streambuf-derived buffer for use with
// JStreamLog.
// Author:	Craig Bookwalter
// Date: 	Aug 2005
// Notes:	Much of this was derived from examples given by
// 	D. Kuehl at http://www.inf.uni-konstanz.de/~kuehl/iostream/
// 	Also, many thanks to J. Hardie for his assistance with 
//  obscure protected-member rules. 
//*************************************************************

#ifndef _DSTREAMLOGBUFFER_H_
#define _DSTREAMLOGBUFFER_H_

#include <iostream>
#include <string.h>
#include <pthread.h>
#include <string>
#include <sstream>
#include <map>

// The following is here just so we can use ROOT's THtml class to generate documentation.
#include "cint.h"

extern pthread_mutex_t jstreamlog_mutex;


/// JStreamLogBuffer is a streambuf-derived buffer for use
/// with JLogStream. Unless you're writing your own stream,
/// you shouldn't ever need this class. It's basically a 
/// wrapper for a passed-in streambuf with the tweak that
/// it appends a prefix to each output statement, namely
/// a status label and a timestamp. 
///

class JStreamLogBuffer : public std::streambuf
{
	private:
		std::streambuf* __sbuf;
		char*			__tag;
//		bool			__newline;
		bool			__prepend_timestamp;
		bool			__prepend_threadstamp;
		
	protected:
		int overflow(int c);
		int sync();
		std::string getThreadStamp();
		std::string getTimeStamp();
		
		// keep a buffer for each thread that tries to write so only
		// complete messages are written to the screen.
		std::map<pthread_t,std::string> thr_buffers;
	
	public:
		JStreamLogBuffer(std::streambuf* buf, const char* tag);
		virtual ~JStreamLogBuffer();

		std::string GetTag(void){return std::string(__tag);}
		bool GetTimestampFlag(void){return __prepend_timestamp;}
		bool GetThreadstampFlag(void){return __prepend_threadstamp;}
		void SetTag(std::string tag);
		void SetTimestampFlag(bool prepend_timestamp=true){__prepend_timestamp=prepend_timestamp;}
		void SetThreadstampFlag(bool prepend_threadstamp=true){__prepend_threadstamp=prepend_threadstamp;}
};

#endif //_DSTREAMLOGBUFFER_H_
