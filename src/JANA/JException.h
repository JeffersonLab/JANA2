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

#ifndef DEXCEPTION_H
#define DEXCEPTION_H

#include <iostream>
#include <exception>
#include <cstdlib>
#include <string>
#include <sstream>
#include <new>

#ifdef __linux__
#include <execinfo.h>
#endif

#include <unistd.h>
#include <limits.h>

class JException : public std::exception 
{
	public :
		JException(std::string msg="");
		virtual	~JException() throw();
		const char* what() const throw();
		const char* trace() const throw();
		friend std::ostream& 
			operator<<(std::ostream& os, const JException& d);
		
	private:
		void getTrace() throw();
		std::string _msg;
		std::string _trace;
};	
	
#endif //DEXCEPTION_H
