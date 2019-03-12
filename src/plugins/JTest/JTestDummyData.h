// $Id$
//
//    File: jana_test.h
// Created: Mon Oct 23 22:39:14 EDT 2017
// Creator: davidl (on Darwin harriet 15.6.0 i386)
//

#ifndef _jana_test_
#define _jana_test_

#include <vector>
#include <utility>
#include <string>
#include <array>

#include <JANA/JObject.h>
#include "JLogger.h"

class JTestDummyData : public JObject
{
	public:
		JTestDummyData(void) : mE(0.0), mID(0) { }

		int GetID(void) const{return mID;}
		double GetE(void) const{return mE;}
		std::vector<double> GetRandoms(void) const{return mRandoms;}

		//SETTERS
		void SetID(int aID){mID = aID;}
		void SetE(double aE){mE = aE;}
		void AddRandom(double aRandom){mRandoms.push_back(aRandom);}

	private:

		std::vector<double> mRandoms;
		std::array<double, 20> mGarbage = {}; //Simulate a "large" object

		double mE;
		int mID;
};

inline JLog& operator<<(JLog& aLog, const JTestDummyData& aObject)
{
  aLog << "Hit: ID=" << aObject.GetID() << ", E=" << aObject.GetE()
       << ", nrandoms=" << aObject.GetRandoms().size();
	return aLog;
}

#endif // _jana_test_
