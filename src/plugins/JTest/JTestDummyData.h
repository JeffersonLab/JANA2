#ifndef _jana_test_
#define _jana_test_

#include <vector>
#include <utility>
#include <string>
#include <array>

#include <JANA/JObject.h>
#include "JLogger.h"

class JTestDummyData : public JObject {

public:

	JTestDummyData(void) : mE(0.0), mID(0) {}

	int GetID(void) const { return mID; }
	void SetID(int aID) { mID = aID; }

	double GetE(void) const { return mE; }
	void SetE(double aE) { mE = aE; }

	std::vector<double> GetRandoms(void) const { return mRandoms; }
	void AddRandom(double aRandom) { mRandoms.push_back(aRandom); }

private:

	std::vector<double> mRandoms;
	std::array<double, 20> mGarbage = {}; //Simulate a "large" object

	double mE;
	int mID;
};

inline JLog &operator<<(JLog &aLog, const JTestDummyData &aObject) {
	aLog << "Hit: ID=" << aObject.GetID() << ", E=" << aObject.GetE()
		 << ", nrandoms=" << aObject.GetRandoms().size();
	return aLog;
}

#endif // _jana_test_
