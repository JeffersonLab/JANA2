//
// JStreamLogBuffer.cc - implementation of the streambuf used by JStreamLog
// Author: Craig Bookwalter
// Date: Aug 2005
//

#include "JStreamLogBuffer.h"
#include <stdio.h>
#include <sstream>

JStreamLogBuffer::JStreamLogBuffer(std::streambuf* buf, const char* tag) :
std::streambuf(),
__sbuf(buf),
__tag(strcpy(new char[strlen(tag)+1], tag)),
__newline(true)
{
	setp(0,0);
	setg(0,0,0);
}

JStreamLogBuffer::~JStreamLogBuffer()
{
	delete[] __tag;
}

int JStreamLogBuffer::overflow(int c) {
	std::stringstream stamp_str;
	stamp_str << "<" << __tag << " @ " << getTimeStamp() << "> : ";
	const char* stamp = stamp_str.str().c_str();
	int len = strlen(stamp);
	int rc = 0;
	if (c != EOF) {
		if (__newline) {
			if (__sbuf->sputn(stamp, len) != len)
				return EOF;
			else
				__newline = false;
		}
		if (c == 6) {
			rc = __sbuf->sputc('\n');
			__newline = true;
		}
		else
			rc = __sbuf->sputc(c);
			
		return rc;
	}
	return 0;
}

int JStreamLogBuffer::sync() {
	return static_cast<JStreamLogBuffer*>(__sbuf)->sync();
}

const char* JStreamLogBuffer::getTimeStamp() {
	time_t thetime;
	time(&thetime);
	char* timestr = ctime(&thetime);
	int len = strlen(timestr);
	timestr[len-1] = '\0';
	return timestr;
}
