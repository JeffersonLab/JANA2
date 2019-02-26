#ifndef _JLog_h_
#define _JLog_h_

#include <vector>
#include <string>
#include <sstream>
#include <cstddef>

// The following is here since it defines an operator<< that causes
// a "note" to be printed from the compiler if the below JLog&&
// operator<< is defined first.
#include <cxxabi.h>


#include "JApplication.h"

/**************************************************************** TYPE DECLARATIONS ****************************************************************/

class JLog;
struct JLogEnd;

/**************************************************************** FUNCTION DECLARATIONS ****************************************************************/

//STREAM OPERATORS FOR RVALUES (TEMPORARIES)
template <typename ArgType>
JLog&& operator<<(JLog&& aLog, ArgType&& aArg);
void operator<<(JLog&& aLog, const JLogEnd&);

//STREAM OPERATORS FOR LVALUES
template <typename ArgType>
JLog& operator<<(JLog& aLog, const ArgType& aArg);
void operator<<(JLog& aLog, const JLogEnd&);

/**************************************************************** TYPE DEFINITIONS ****************************************************************/

struct JLogEnd { };

class JLog
{
	public:

		//STRUCTORS
		JLog(uint32_t aLogIndex = 0) { }

		//HEADERS
		void SetHeaders(const std::vector<std::string>& aHeaders){mHeaders = aHeaders;}
		void GetHeaders(std::vector<std::string>& aHeaders){aHeaders = mHeaders;}

		//FRIENDS: FOR RVALUES
		template <typename ArgType>
		friend JLog&& operator<<(JLog&& aLog, ArgType&& aArg);
		friend void operator<<(JLog&& aLog, const JLogEnd&);

		//FRIENDS: FOR LVALUES
		template <typename ArgType>
		friend JLog& operator<<(JLog& aLog, const ArgType& aArg);
		friend void operator<<(JLog& aLog, const JLogEnd&);

	private:
		std::vector<std::string> mHeaders;
		std::ostringstream mStream;
};

/**************************************************************** FUNCTION DEFINITIONS ****************************************************************/

//Two different versions: passing in JLog by lvalue reference and by rvalue reference
//e.g.: lvalue reference is reference to std::cout << ...
//e.g. rvalue reference is reference to a temporary: JLog() << ... (temporary because no name (constructed/used inline))

template <typename ArgType>
JLog&& operator<<(JLog&& aLog, ArgType&& aArg)
{
	//defer to lvalue version
	aLog << std::forward<ArgType>(aArg);
	return std::move(aLog); //aLog is an lvalue (it has an address) that refers to an rvalue: move to make a reference again
}

inline void operator<<(JLog&& aLog, const JLogEnd& aLogEnd)
{
	//defer to lvalue version
	aLog << aLogEnd;
}

template <typename ArgType>
JLog& operator<<(JLog& aLog, const ArgType& aArg)
{
	aLog.mStream << aArg;
	return aLog;
}

inline void operator<<(JLog& aLog, const JLogEnd&)
{
	std::cout << aLog.mStream.str();
	aLog.mStream.str(""); //in case re-used
	return;
}

#endif
