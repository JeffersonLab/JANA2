#ifndef _JLogWrapper_h_
#define _JLogWrapper_h_

#include <string>
#include <mutex>
#include <ostream>

/**************************************************************** TYPE DECLARATIONS ****************************************************************/

class JLogWrapper;

/**************************************************************** TYPE DEFINITIONS ****************************************************************/

class JLogWrapper
{
	public:
		JLogWrapper(std::ostream& aStream, const std::string& aLogPrefix = "", const std::string& aLogSuffix = "") : mStream(aStream), mLogPrefix(aLogPrefix), mLogSuffix(aLogSuffix) { }
		void Stream(const std::string& aContent);

	private:
		std::ostream& mStream;
		std::string mLogPrefix;
		std::string mLogSuffix;
		std::mutex mMutex;
};

/**************************************************************** MEMBER FUNCTION DEFINITIONS ****************************************************************/

inline void JLogWrapper::Stream(const std::string& aContent)
{
	std::lock_guard<std::mutex> sLock(mMutex);
	mStream << mLogPrefix << aContent << mLogSuffix << std::flush;
}

#endif
