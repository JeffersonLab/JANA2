// JException.h

// Base exception class for JANA package

// EJW, JLab, 19-apr-2007

///
/// JException.cc - implementation of DANA exceptions with rudimentary stack 
/// traces. See JException.h for advice regarding usage.
/// 
/// Author: Craig Bookwalter (craigb at jlab.org)
/// Date:   March 2006
///

#include "JException.h"

#include <stdlib.h>
#include <string.h>

using namespace std;
using namespace jana;

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------


/** 
 * Constructor given text field.
 *
 * @param txt Exception text
 */
JException::JException(const string &txt) : text(txt), code(0), source(""), trace(getStackTrace()) {}


//-----------------------------------------------------------------------------


/** 
 * Constructor given text field and source file info.
 *
 * @param txt Exception text
 * @param file Name of file where exception raised
 * @param line Line in file where exception raised
 */
JException::JException(const string &txt, const char *file, int line) : text(txt), code(0), trace(getStackTrace()) {
  stringstream ss;
  ss << "File: " << file << ",   Line: " << line << ends;
  source=ss.str();
}


//-----------------------------------------------------------------------------


/** 
 * Constructor given text field and code.
 *
 * @param txt Exception text
 * @param c   Exception code
 */
JException::JException(const string &txt, int c) : text(txt), code(c), source(""), trace(getStackTrace()) {}


//-----------------------------------------------------------------------------


/** 
 * Constructor given text field, code, and source file info.
 *
 * @param txt Exception text
 * @param c   Exception code
 * @param file Name of file where exception raised
 * @param line Line in file where exception raised
 */
JException::JException(const string &txt, int c, const char *file, int line) : text(txt), code(c), trace(getStackTrace()) {
  stringstream ss;
  ss << "File: " << file << ",   Line: " << line << ends;
  source=ss.str();
}


//-----------------------------------------------------------------------------


/** 
 * Returns string representation of exception including trace.
 *
 * @return String representation of exception
 */
string JException::toString(void) const throw() {
  return(toString(true));
}


//-----------------------------------------------------------------------------


/** 
 * Returns string representation of exception.
 *
 * @param includeTrace true to include trace info
 *
 * @return String representation of exception
 */
string JException::toString(bool includeTrace) const throw() {
  ostringstream oss;
  oss << endl << "?JException:    code = " << code << "    text = " << text << endl << endl;
  if(source.size()>0) oss << source << endl << endl;
  if(includeTrace&&(trace.size()>0))  oss << "Stack trace:" << endl << endl << trace << endl;
  oss << ends;
  return(oss.str());
}


//-----------------------------------------------------------------------------


/** 
 * Returns char* representation of exception.
 *
 * @return char* representation of exception
 */
const char *JException::what(void) const throw() {
  return(toString().c_str());
}


//-----------------------------------------------------------------------------


/** 
 * Generates stack trace using Linux system calls, doesn't work on solaris.
 *
 * @return String containing stack trace
 */
string JException::getStackTrace(void) {

#ifndef TRACEABLE_OS
	return string("");
#else
  size_t dlen;
  char dname[1024];
  void *trace[1024];
  int status;
  
  
  // get trace messages
  int trace_size = backtrace(trace,1024);
  if(trace_size>1024)trace_size=1024;
  char **messages = backtrace_symbols(trace, trace_size);
  
  // de-mangle and create string
  stringstream ss;
  for(int i=0; i<trace_size; ++i) {
    
    // find first '(' and '+'
    char *ppar  = strchr(messages[i],'(');
    char *pplus = strchr(messages[i],'+');
    if((ppar!=NULL)&&(pplus!=NULL)) {
      
      // replace '+' with nul, then get de-mangled name
      *pplus='\0';
      abi::__cxa_demangle(ppar+1,dname,&dlen,&status);
      
      // add to stringstream
      *(ppar+1)='\0';
      *pplus='+';
      ss << "   " << messages[i] << dname << pplus << endl;
      
    } else {
      ss << "   " << messages[i] << endl;
    }
    
  }
  ss << ends;
  
  free(messages);
  return(ss.str());

#endif // TRACEABLE_OS
}


//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
#if 1
std::ostream& jana::operator<<(std::ostream& os, const jana::JException& d) {
	os << d.toString(true);
	return os;
}
#endif
// Below here is Craig's original code (minus the stream operator above). It was
// disabled when merging it with Elliott's code since they overlapped so much.
// I'm keeping it here for a little while at least because:
//
// 1. Craig's original JException was the only one ever used so far in the JANA/DANA code
//
// 2. His getTrace() method works in a slightly different way than Elliott's
// which may prove useful once we get more experience using this class.
#if 0


JException::JException(std::string msg) :
_msg(msg),
_trace("")
{
	#ifdef __linux__	
		getTrace();
	#endif
}

JException::~JException() throw () {}

const char* JException::what() const throw() {
	return _msg.c_str();
}

void JException::getTrace() throw() {
#ifdef __linux__
	void* traces[25];
	FILE* psOutput;
	FILE* addr2lineOutput;
	FILE* whichOutput;
	std::stringstream sstemp; 
	std::string path;
	std::string prim_loc = "";
	std::string fullTrace;
	int nLevels;
	char temp_ch;
	char* myName = new(std::nothrow) char [NAME_MAX];	// NAME_MAX is defined in limits.h
	bool firstLine = true;
	bool newlineEncountered = false;
	nLevels = backtrace(traces, 25);

	// if nLevels == size of the array, it's possible that the depth of
	// the stack exceeds the size of the array, and you won't get a full trace.
	if (nLevels == 25) {}
		// Put in global error log message here. 
	
	// if myName == NULL, the new(nothrow) above failed to allocate memory.
	if (!myName) {}
		// Put in global error log message here
	
	// All of this output is subject to redirection to error logs.	
	
	
	// First, we need to get the name of this executable. Apparently, there is no 
	// system call for this. So, the man page for "ps" says:
	// Print only the name of PID 42:
    // $> ps -p 42 -o comm=
    
	sstemp << "ps -p " << getpid() << " -o comm=" << std::endl;
	psOutput = popen(sstemp.str().c_str(), "r");
	for (int i=0; ((temp_ch = getc(psOutput)) != EOF) || i > NAME_MAX; i++) 
		myName[i] = temp_ch;
	myName[strlen(myName)-1] = '\0';
	pclose(psOutput);
	sstemp.str("");
	
	
	// Now, we have to figure out the path of the executable, using "which" and 
	// a few shenanigans with environment variables. Note that if you're doing
	// something funny on the command line, you'll likely fool this algorithm.
	sstemp << getenv("PATH");
	path = sstemp.str();
	sstemp.str("");
	sstemp << ".:" << path;
	setenv("PATH",sstemp.str().c_str(),1);
	sstemp.str("");
	sstemp << "which " << myName;
	whichOutput = popen(sstemp.str().c_str(), "r");
	for (int i=0; ((temp_ch = getc(whichOutput)) != EOF) || i > NAME_MAX; i++)
		myName[i] = temp_ch;
	
	myName[strlen(myName)-1] = '\0';
	pclose(whichOutput);
	sstemp.str("");
	
	// Next, we ask "addr2line" of the GNU binary utilities package for 
	// information about the names of the addresses we got from backtrace().

	sstemp << "addr2line -e " << myName << " ";	
		
	for (int i=0; i < nLevels; i++) 
		sstemp << traces[i] << " ";
	addr2lineOutput	= popen(sstemp.str().c_str(), "r");
	sstemp.str("");
	
	// Pretty up the output.
		
	for (int i=0; ((temp_ch = getc(addr2lineOutput)) != EOF); i++) {
		if (temp_ch == '?')
			break;
		if (newlineEncountered) {
			newlineEncountered = false;
			sstemp << '\t';
		}
		if (temp_ch == '\n') {
			// Skip the first line, because it always references this file.
			if (firstLine) {  
				firstLine = false;
				sstemp.str("");
				continue;
			}
			else if (prim_loc == "") { 
				prim_loc = sstemp.str();
				sstemp.str("");
				continue;
			}
			else 
				newlineEncountered = true;	
		}
		sstemp << temp_ch;
	}
	
	fullTrace = sstemp.str();
	sstemp.str("");
	
	if (_msg != "")
		sstemp		<< "Exception (\"" << _msg << "\") thrown at:\n";
	else
		sstemp 		<< "Exception thrown at:\n";

			
	sstemp		<< "\t" << prim_loc << "\n";
	sstemp 		<< "referenced by: " << "\n" 
				<< "\t" << fullTrace << std::endl; 
	
	_trace = sstemp.str();
	delete myName;	
	pclose(addr2lineOutput);
	setenv("PATH", path.c_str(), 1);
#else // __linux__
	_trace = std::string("backtrace available only on Linux platforms");
#endif // __linux__
}

const char* JException::trace() const throw() {
	return _trace.c_str();
}

std::ostream& operator<<(std::ostream& os, const JException& d) {
	os << d.trace;
	return os;
}

#endif // 0
