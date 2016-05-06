
#include <pthread.h>

//using namespace std;

#include "JStreamLog.h"

JStreamLog jout(std::cout, "JANA >>");
JStreamLog jerr(std::cerr, "JANA ERROR>>");

JStreamLog::JStreamLog(std::streambuf* buf, const char* tag) : std::ostream(new JStreamLogBuffer(buf, tag)), own_rdbuf(true)
{}

JStreamLog::JStreamLog(const std::ostream& os, const char* tag) : std::ostream(new JStreamLogBuffer(os.rdbuf(), tag)), own_rdbuf(true)
{}

JStreamLog::JStreamLog(const std::ostream& os, const std::string& tag) : std::ostream(new JStreamLogBuffer(os.rdbuf(), tag.c_str())), own_rdbuf(true)
{}

JStreamLog::~JStreamLog() {
	// On some Linux systems (CentOS 6.4) it seems this destructor
	// is called twice whenever a plugin is attached. I can only guess
	// that the global-scope jout and jerr are somehow linked when 
	// the plugin is attached, but their destructors are called both
	// when they are detached and when the program exists. The problem
	// manifests in a seg. fault at the very end of the program. To avoid
	// this, we keep a flag that is set in the constructor and cleared
	// here in the destructor to indicate that the JStreamLogBuffer has 
	// been deleted already. Thus, we don't try deleting it a second time
	// and therefore avoid the seg. fault.    Dec. 13, 2013  D.L.
	if(own_rdbuf){
		std::streambuf *mybuf = rdbuf(NULL);
		if(mybuf!=NULL) delete mybuf;
	}
	own_rdbuf = false;
}

std::ostream& endMsg(std::ostream& dSL) {
	dSL << static_cast<char>(6); 
	dSL << std::flush;
	return dSL;
}

//------------------
// GetTag
//------------------
std::string JStreamLog::GetTag(void)
{
	JStreamLogBuffer *b = GetJStreamLogBuffer();
	
	return (b ? b->GetTag():"unknown");
}

//------------------
// GetTimestampFlag
//------------------
bool JStreamLog::GetTimestampFlag(void)
{
	JStreamLogBuffer *b = GetJStreamLogBuffer();
	
	return (b ? b->GetTimestampFlag():false);
}

//------------------
// GetThreadstampFlag
//------------------
bool JStreamLog::GetThreadstampFlag(void)
{
	JStreamLogBuffer *b = GetJStreamLogBuffer();
	
	return (b ? b->GetThreadstampFlag():false);
}

//------------------
// SetTag
//------------------
void JStreamLog::SetTag(std::string tag)
{
	JStreamLogBuffer *b = GetJStreamLogBuffer();
	if(b)b->SetTag(tag);
}

//------------------
// SetTimestampFlag
//------------------
void JStreamLog::SetTimestampFlag(bool prepend_timestamp)
{
	JStreamLogBuffer *b = GetJStreamLogBuffer();
	if(b)b->SetTimestampFlag(prepend_timestamp);
}

//------------------
// SetThreadstampFlag
//------------------
void JStreamLog::SetThreadstampFlag(bool prepend_threadstamp)
{
	JStreamLogBuffer *b = GetJStreamLogBuffer();
	if(b)b->SetThreadstampFlag(prepend_threadstamp);
}

//------------------
// GetJStreamLogBuffer
//------------------
JStreamLogBuffer* JStreamLog::GetJStreamLogBuffer(void)
{
	// Try and dynamic cast our streambuf as a JStreamLogBuffer
	return dynamic_cast<JStreamLogBuffer*> (rdbuf());
}


