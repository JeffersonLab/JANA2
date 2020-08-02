//
// JStreamLogBuffer.cc - implementation of the streambuf used by JStreamLog
// Author: Craig Bookwalter
// Date: Aug 2005
//

#include "JStreamLogBuffer.h"
#include <stdio.h>
#include <pthread.h>
#include <sstream>
using namespace std;

pthread_mutex_t jstreamlog_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_rwlock_t jstreamlog_rwlock = PTHREAD_RWLOCK_INITIALIZER;

//------------------
// JStreamLogBuffer
//------------------
JStreamLogBuffer::JStreamLogBuffer(std::streambuf* buf, const char* tag) :
std::streambuf(),
__sbuf(buf),
__tag(strcpy(new char[strlen(tag)+1], tag)),
//__newline(true),
__prepend_timestamp(false),
__prepend_threadstamp(false)
{
	// The setp(0,0) call effectively sets the buffer size to 0
	// The reason we set it to 0 is so every character written
	// to the stream will invoke a call to the overflow() method below.
	setp(0,0);
	
	// The setg is for input so not really used here.
	setg(0,0,0);
}

//------------------
// ~JStreamLogBuffer
//------------------
JStreamLogBuffer::~JStreamLogBuffer()
{
	delete[] __tag;
}

//------------------
// overflow
//------------------
int JStreamLogBuffer::overflow(int c)
{
	// Because we called setp(0,0), this should get called for every character
	// written. Because multiple threads may be calling us, we want to sort
	// the characters into buffers, one for each thread. This way, we can prevent
	// 2 threads from writing messages "on top" of one another so that it comes
	// out as gibbersih.
	//
	// We use a rwlock here to hopefully minimize the perfomance hit we take for
	// locking/unlocking for every character written.

	pthread_rwlock_rdlock(&jstreamlog_rwlock);
	map<pthread_t,string>::iterator iter = thr_buffers.find(pthread_self());
	if(iter==thr_buffers.end()){
		// Add this thread to our list, creating a string for it
		pthread_rwlock_unlock(&jstreamlog_rwlock);	// release read only lock
		pthread_rwlock_wrlock(&jstreamlog_rwlock);	// obtain write lock
		thr_buffers[pthread_self()] = "";
		pthread_rwlock_unlock(&jstreamlog_rwlock);	// release write lock
		pthread_rwlock_rdlock(&jstreamlog_rwlock);	// re-obtain read only lock
	}

	// Get reference to this thread's string
	string &str = thr_buffers[pthread_self()];
	
	// Add this character to the string
	str += (char)c;
	
	// Check if "c" is a special value warranting a flush to __sbuf
	string copy_out;
	switch(c){
		case '\n':
		case '\r':
			copy_out = str;
			str.clear(); // reset the streamstring
			break;
		default:
			break;
	 } 
	
	// Release read lock
	pthread_rwlock_unlock(&jstreamlog_rwlock);
	
	// If we have a string to copy, then do it
	if(copy_out.size()!=0){
	
		// Prepend tag (if present)
		if(strlen(__tag)>0)copy_out.insert(0, string(__tag));

		// Optionally prepend threadid
		if(__prepend_threadstamp)copy_out.insert(0, getThreadStamp()+" # ");
	
		// Optionally prepend time
		if(__prepend_timestamp)copy_out.insert(0, getTimeStamp()+" # ");
	
		// Use the mutex to prevent more than one thread from writing at a time
		pthread_mutex_lock(&jstreamlog_mutex);

		const char *a = copy_out.c_str();
		for(unsigned int i=0; i<copy_out.size(); i++, a++)__sbuf->sputc(*a);

		pthread_mutex_unlock(&jstreamlog_mutex);
	}

	return 0;
}

//------------------
// sync
//------------------
int JStreamLogBuffer::sync() {
	return static_cast<JStreamLogBuffer*>(__sbuf)->sync();
}

//------------------
// getThreadStamp
//------------------
string JStreamLogBuffer::getThreadStamp() {
	stringstream ss;
	ss<<"thr="<<pthread_self();
	return ss.str();
}

//------------------
// getTimeStamp
//------------------
string JStreamLogBuffer::getTimeStamp() {
	time_t thetime;
	time(&thetime);
	char* timestr = ctime(&thetime);
	int len = strlen(timestr);
	timestr[len-1] = '\0';
	return string(timestr);
}

//------------------
// SetTag
//------------------
void JStreamLogBuffer::SetTag(string tag)
{
	delete[] __tag;
	const char *ctag = tag.c_str();
	__tag = new char[strlen(ctag)+1];
	strcpy(__tag, ctag);
}

