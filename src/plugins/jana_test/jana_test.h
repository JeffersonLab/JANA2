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
#include "JLog.h"

class jana_test : public JObject
{
	public:
		JOBJECT_PUBLIC(jana_test);
		
		//STRUCTORS
		jana_test(double aE, int aID) : mE(aE), mID(aID) { }

		//GETTERS
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

//STREAM OPERATOR
inline JLog& operator<<(JLog& aLog, const jana_test& aObject)
{
	aLog.SetHeaders({"ID", "E", "#Randoms"});
	aLog << aObject.GetID() << aObject.GetE() << aObject.GetRandoms().size() << "\n";
	return aLog;
}

#endif // _jana_test_
