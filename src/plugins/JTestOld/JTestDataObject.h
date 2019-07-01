#pragma once

#include <vector>
#include <utility>
#include <string>
#include <array>

#include <JANA/JObject.h>
#include "JLogger.h"

// We define all of our JObjects in one place.
// TODO: Can we do a tagged Get() so that we don't have to copy-paste datatype definitions like this?
// TODO: Can we get rid of the mGarbage field which doesn't do anything other than generate compiler warnings?

// JTestDataObject is the only one which gets 'calculated' via a Factory;
// the other two come from EventSource.GetObjects()

struct JTestDataObject : public JObject {

	int mID = 0;
	double mE = 0.0;
	std::vector<double> mRandoms;
	std::array<double, 20> mGarbage = {}; //Simulate a "large" object

	friend JLog &operator<<(JLog &aLog, const JTestDataObject &aObject) {
		aLog << "JTestDataObject: ID=" << aObject.mID << ", E=" << aObject.mE
			 << ", nrandoms=" << aObject.mRandoms.size();
		return aLog;
	}
};

struct JTestSourceData1 : public JObject {

	int mID = 0;
	double mE = 0.0;
	std::vector<double> mRandoms;
	std::array<double, 20> mGarbage = {}; //Simulate a "large" object

	JTestSourceData1(double aE, double aID) : mID(aID), mE(aE) {}

	friend JLog &operator<<(JLog &aLog, const JTestSourceData1 &aObject) {
		aLog << "JTestSourceData1: ID=" << aObject.mID << ", E=" << aObject.mE
			 << ", nrandoms=" << aObject.mRandoms.size();
		return aLog;
	}
};

struct JTestSourceData2 : public JObject {

	int mID = 0;
	double mE = 0.0;
	std::vector<double> mRandoms;
	std::array<double, 20> mGarbage = {}; //Simulate a "large" object

	JTestSourceData2(double aE, double aID) : mID(aID), mE(aE) {}

	friend JLog &operator<<(JLog &aLog, const JTestSourceData2 &aObject) {
		aLog << "JTestSourceData2: ID=" << aObject.mID << ", E=" << aObject.mE
			 << ", nrandoms=" << aObject.mRandoms.size();
		return aLog;
	}
};
