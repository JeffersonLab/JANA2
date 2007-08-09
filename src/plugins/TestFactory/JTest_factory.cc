// $Id$
//
//    File: JTest_factory.cc
// Created: Wed Aug  8 20:52:23 EDT 2007
// Creator: davidl (on Darwin Amelia.local 8.10.1 i386)
//

#include <cmath>

#include "JTest_factory.h"

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
jerror_t JTest_factory::brun(JEventLoop *eventLoop, int runnumber)
{
	val1 = 1.234;
	val2 = 9.876;

	return NOERROR;
}

//------------------
// evnt
//------------------
jerror_t JTest_factory::evnt(JEventLoop *loop, int eventnumber)
{

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
		for(int j=0; j<100000; j++){
			a = log(fabs(a*sqrt(pow(a, 2.2))));
		}
		
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

//------------------
// toString
//------------------
const string JTest_factory::toString(void)
{
	// Ensure our Get method has been called so _data is up to date
	Get();
	if(_data.size()<=0)return string(); // don't print anything if we have no data!

	// Put the class specific code to produce nicely formatted ASCII here.
	// The DFactory_base class has several methods defined to help. They
	// rely on positions of colons (:) in the header. Here's an example:
	//
	printheader("row:    x:     y:");
	
	for(unsigned int i=0; i<_data.size(); i++){
		JTest *myJTest = _data[i];
	
		printnewrow();
		printcol("%d",	i);
		printcol("%1.3f",	myJTest->x);
		printcol("%3.2f",	myJTest->y);
		printrow();
	}

	return _table;
}
