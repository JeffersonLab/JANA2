// $Id$
//
//    File: JSourceObject.h
// Created: Mon Oct 23 22:39:14 EDT 2017
// Creator: davidl (on Darwin harriet 15.6.0 i386)
//

#ifndef _JSourceObject_
#define _JSourceObject_

#include <vector>
#include <utility>
#include <string>
#include <array>

#include <JANA/JObject.h>
#include "JLog.h"

class JSourceObject : public JObject
{
	public:
		JOBJECT_PUBLIC(JSourceObject);
		
		//STRUCTORS
		JSourceObject(double aHitE, int aHitID) : mHitE(aHitE), mHitID(aHitID) { }

		//GETTERS
		int GetHitID(void) const{return mHitID;}
		double GetHitE(void) const{return mHitE;}
		std::vector<double> GetRandoms(void) const{return mRandoms;}

		//SETTERS
		void SetHitID(int aHitID){mHitID = aHitID;}
		void SetHitE(double aHitE){mHitE = aHitE;}
		void AddRandom(double aRandom){mRandoms.push_back(aRandom);}

	private:

		std::vector<double> mRandoms;
		std::array<double, 20> mGarbage = {}; //Simulate a "large" object

		double mHitE;
		int mHitID;
};

//STREAM OPERATOR
inline JLog& operator<<(JLog& aLog, const JSourceObject& aObject)
{
	aLog.SetHeaders({"Hit ID", "Hit E", "Hit #Randoms"});
	aLog << aObject.GetHitID() << aObject.GetHitE() << aObject.GetRandoms().size() << "\n";
	return aLog;
}

#endif // _JSourceObject_
