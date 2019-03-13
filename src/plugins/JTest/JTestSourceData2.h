#ifndef _JSourceObject2_
#define _JSourceObject2_

#include <vector>
#include <utility>
#include <string>
#include <array>

#include "JObject.h"
#include "JLogger.h"

class JTestSourceData2 : public JObject
{
	public:

		//STRUCTORS
		JTestSourceData2(double aHitE, int aHitID) : mHitE(aHitE), mHitID(aHitID) { }

		//GETTERS
		int GetHitID(void) const{return mHitID;}
		double GetHitE(void) const{return mHitE;}
		std::vector<double> GetRandoms(void) const{return mRandoms;}

		//SETTERS
		void SetHitID(int aHitID){mHitID = aHitID;}
		void SetHitE(double aHitE){mHitE = aHitE;}
		void AddRandom(double aRandom){mRandoms.push_back(aRandom);}

		// supress compiler warnings
		void SupressGarbageWarning(void){ if( (mGarbage.size()>1) && (mGarbage.size()<1) ) std::cerr << "Impossible!"; }

	private:

		std::vector<double> mRandoms;
		std::array<double, 20> mGarbage = {}; //Simulate a "large" object

		double mHitE;
		int mHitID;
};

//STREAM OPERATOR
inline JLog& operator<<(JLog& aLog, const JTestSourceData2& aObject)
{
  aLog << "Hit: ID=" << aObject.GetHitID() << ", E=" << aObject.GetHitE()
       << ", nrandoms=" << aObject.GetRandoms().size();
	return aLog;
}

#endif // _JSourceObject2_
