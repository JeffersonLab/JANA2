#ifndef _JLog_h_
#define _JLog_h_

#include <vector>
#include <string>
#include <sstream>
#include <cstddef>

#include "JApplication.h"
#include "JLogWrapper.h"

/**************************************************************** TYPE DECLARATIONS ****************************************************************/

class JLog;
struct JLogEnd;

/**************************************************************** FUNCTION DECLARATIONS ****************************************************************/

template <typename ArgType>
JLog& operator<<(JLog& aLog, const ArgType& aArg);

void operator<<(JLog& aLog, const JLogEnd&);

/**************************************************************** TYPE DEFINITIONS ****************************************************************/

struct JLogEnd { };
struct JLogLineEnd { };

class JLog
{
	public:

		//STRUCTORS
		JLog(uint32_t aLogIndex = 0) : mLogIndex(aLogIndex), mLogWrapper(japp->GetLogWrapper(mLogIndex)) { }

		//HEADERS
		void SetHeaders(const std::vector<std::string>& aHeaders){mHeaders = aHeaders;}
		void GetHeaders(std::vector<std::string>& aHeaders){aHeaders = mHeaders;}

		//FRIENDS
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
	aLog.mStream.str(""); //in case re-used
}

#endif
