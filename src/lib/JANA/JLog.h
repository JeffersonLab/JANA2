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
#include "JLogWrapper.h"

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
		JLog(uint32_t aLogIndex = 0) : mLogIndex(aLogIndex), mLogWrapper(japp->GetLogWrapper(mLogIndex)) { }

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
		std::size_t mHeaderIndex = 0;
		uint32_t mLogIndex = 0;
		std::ostringstream mStream;
		JLogWrapper* mLogWrapper = nullptr;
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
	if(aLog.mLogIndex == 2) //For hd_dump
	{
		if((aLog.mHeaderIndex != 0) && (aLog.mHeaderIndex < aLog.mHeaders.size())) //check at end: for \n
			aLog.mStream << " "; //format spacing here!!
		aLog.mStream << aArg;
		aLog.mHeaderIndex++;
		if(aLog.mHeaderIndex > aLog.mHeaders.size())
			aLog.mHeaderIndex = 0; //reset for next object
		return aLog;
	}

	if(aLog.mHeaderIndex < aLog.mHeaders.size())
	{
		if(aLog.mHeaderIndex != 0)
			aLog.mStream << ", ";
		aLog.mStream << aLog.mHeaders[aLog.mHeaderIndex++] << ": ";
	}
	else
	{
		//We've used up the headers, clear them so that future calls aren't confused
		aLog.mHeaders.clear();
		aLog.mHeaderIndex = 0; //reset for next object
	}

	aLog.mStream << aArg;
	return aLog;
}

inline void operator<<(JLog& aLog, const JLogEnd&)
{
	if(aLog.mLogIndex != 2) //For non-hd_dump
	{
		aLog.mLogWrapper->Stream(aLog.mStream.str());
		aLog.mStream.str(""); //in case re-used
		return;
	}

	//Format headers first
	std::ostringstream sStream;
	auto sNumHeaders = aLog.mHeaders.size();
	for(std::size_t si = 0; si < sNumHeaders; si++)
	{
		sStream << aLog.mHeaders[si];
		if(si != (sNumHeaders - 1))
			sStream << " ";
	}
	//End header line & add content:
	auto sLength = sStream.str().size();
	sStream << "\n" << std::string(sLength, '-') << '\n';

	//Add content & Send message
	sStream << aLog.mStream.str();
	aLog.mLogWrapper->Stream(sStream.str());

	//Reset in case re-used
	aLog.mStream.str("");
	aLog.mHeaderIndex = 0;
}

#endif
