

// JException2.h

// Base exception class for JANA package

// EJW, JLab, 19-apr-2007
/// 
/// JException - exception definition for DANA
/// Author:  Craig Bookwalter (craigb at jlab.org) 
/// Date:    December 2005
/// Usage: 
/// 	
///     #include <JException.h>
///
///     void someFunction() {
///	        ...
///         throw JException("Error details.");
///		}
///
///	 Notes:	
///   - you must compile with the -g option (g++) to get readable output
///	  - you must be running an executable in the current directory or from
///	  some directory on your path in order for exceptions to function fully
///	  - you must catch the exception to view the stack trace (using the
///   what() method). This encourages proper try/catch structure. 
///	  - you can write JExceptions to any stream, including JLogStreams if
///   you wish to keep a log of exceptions. 
/// 
///	 To do:
///	  - protect against executables that cannot be located
///   - respond intelligently to executables not compiled with debug info
///   - test and adapt to Solaris platform
///	  - make it thread-safe 	
///

#ifndef _JException_
#define _JException_

#include <string>

// The following is here just so we can use ROOT's THtml class to generate documentation.
#include "cint.h"

#if !defined(__CINT__) && !defined(__CLING__)

#include <exception>
#include <sstream>
using std::exception;

#if defined(__linux__) || defined(__APPLE__)
#define TRACEABLE_OS
#include <execinfo.h>
#include <cxxabi.h>
#endif


#endif // __CINT__  __CLING__

// Place everything in JANA namespace
namespace jana{


class JException : public std::exception 
{
	public :
		JException(const std::string &txt);
		JException(const std::string &txt, const char *file, int line);
		JException(const std::string &txt, int c);
		JException(const std::string &txt, int c, const char *file, int line);
		virtual ~JException(void) throw();

		virtual std::string toString(void) const throw();
		virtual std::string toString(bool includeTrace) const throw();
		virtual const char* what(void) const throw();
		static std::string getStackTrace(bool demangle_names=true, size_t max_frames=1024);
		
		//JException(std::string msg="");
		//const char* what() const throw();
		//const char* trace() const throw();
		friend std::ostream& operator<<(std::ostream& os, const jana::JException& d);
		
	private:
		JException(); // disallow default constructor
		std::string text;     /**<Exception text.*/
		int code;        /**<Exception code.*/
		std::string source;   /**<Exception source file info.*/
		std::string trace;    /**<Stack trace generated automatically.  Does not work on all architectures.*/
		//void getTrace() throw();
		//std::string _msg;
		//std::string _trace;
};

std::ostream& operator<<(std::ostream& os, const jana::JException& d);

} // Close JANA namespace



#endif //_JException_
