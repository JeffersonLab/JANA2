
#include <pthread.h>

using namespace std;

#include "JStreamLog.h"

JStreamLog jout(std::cerr, "JANA >>");
JStreamLog jerr(std::cerr, "JANA ERROR>>");

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

//------------------
// GetTag
//------------------
string JStreamLog::GetTag(void)
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
// SetTag
//------------------
void JStreamLog::SetTag(string tag)
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
// GetJStreamLogBuffer
//------------------
JStreamLogBuffer* JStreamLog::GetJStreamLogBuffer(void)
{
	// Try and dynamic cast our streambuf as a JStreamLogBuffer
	return dynamic_cast<JStreamLogBuffer*> (rdbuf());
}


