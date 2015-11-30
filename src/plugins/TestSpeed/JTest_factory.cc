// $Id$
//
//    File: JTest_factory.cc
// Created: Wed Aug  8 20:52:23 EDT 2007
// Creator: davidl (on Darwin Amelia.local 8.10.1 i386)
//

#include <cmath>
#include <iostream>

#include "JTest_factory.h"
#include "JRawData.h"

//------------------
// JTest_factory (constructor)
//------------------
JTest_factory::JTest_factory(void)
{
	// There is a speed governor in the evnt() method that chews up CPU
	// cycles to better emulate an actual system. The governer_iterations
	// parameter controls how much CPU to use. This can be changed on
	// the command-line at program start by adding the option:
	//   -PGOVERNOR_ITERATIONS=###
	// where ### is some integer. Setting ### to 0 will bypass the governor
	// completely.
	governor_iterations = 1000;
	gPARMS->SetDefaultParameter("GOVERNOR_ITERATIONS", governor_iterations);
}

//------------------
// init
//------------------
jerror_t JTest_factory::init(void)
{

	return NOERROR;
}

//------------------
// brun
//------------------
jerror_t JTest_factory::brun(JEventLoop *eventLoop, int32_t runnumber)
{
	val1 = 1.234;
	val2 = 9.876;
	
	return NOERROR;
}

//------------------
// evnt
//------------------
jerror_t JTest_factory::evnt(JEventLoop *loop, uint64_t eventnumber)
{

	// Get JRawData objects from source. In principle, we would use these
	// to do something here, but for this simple example, we simply get
	// them and then don't use them.
	vector<const JRawData*> rawdatas;
	loop->Get(rawdatas);

	// Create a few JTest objects
	for(int i=0; i<100; i++){
		JTest *myJTest = new JTest;
		myJTest->x = std::cos(val1);
		myJTest->y = std::sin(val2);
		
		// do something computationally intensive to
		// slow us down. Most actual factories will
		// need to do a lot of calculations to produce
		// the factory's data.
		double a = 1.234;
		for(int j=0; j<governor_iterations; j++){
			a = log(fabs(a*sqrt(pow(a, 2.2))));
		}
		myJTest->z = a;
		
		val1 *= val2;
		val2 /= val1;
	
		// Push the JTest object pointer onto the _data vector we inherited
		// through JFactory. Note that the objects in _data will be deleted
		// later by the system and the _data vector will be cleared automatically.
		_data.push_back(myJTest);
	}

	return NOERROR;
}

//------------------
// erun
//------------------
jerror_t JTest_factory::erun(void)
{
	return NOERROR;
}

//------------------
// fini
//------------------
jerror_t JTest_factory::fini(void)
{
	return NOERROR;
}
