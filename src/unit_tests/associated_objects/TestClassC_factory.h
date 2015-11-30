// $Id$
//
//    File: TestClassC_factory.h
// Created: Thu May 21 13:30:17 EDT 2015
// Creator: davidl (on Linux gluon47.jlab.org 2.6.32-358.23.2.el6.x86_64 x86_64)
//

#ifndef _TestClassC_factory_
#define _TestClassC_factory_

#include <JANA/JFactory.h>
#include "TestClassB.h"
#include "TestClassC.h"

class TestClassC_factory:public jana::JFactory<TestClassC>{
	public:
		TestClassC_factory(){};
		~TestClassC_factory(){};


	private:
		jerror_t init(void){return NOERROR;}
		jerror_t brun(jana::JEventLoop *eventLoop, int32_t runnumber){return NOERROR;}
		jerror_t erun(void){return NOERROR;}
		jerror_t fini(void){return NOERROR;}

		jerror_t evnt(jana::JEventLoop *loop, uint64_t eventnumber){
		
			vector<const TestClassA*> objsA;
			vector<const TestClassB*> objsB;
			loop->Get(objsA);
			loop->Get(objsB);

			// Create objects of type TestClassC for all but the
			// last TestClassB object and associate them.
			for(uint32_t i=0; i<objsB.size()-1; i++){
				
				TestClassC *objC = new TestClassC;
				objC->AddAssociatedObject(objsB[i]);

				_data.push_back(objC);
			}

			// Add one more TestClassB as an associated object
			// to the last TestClassC object
			_data[_data.size()-1]->AddAssociatedObject(objsB[objsB.size()-1]);

			// Also add one more TestClassA as an associated object
			// to the last TestClassB object
			_data[_data.size()-1]->AddAssociatedObject(objsA[objsB.size()+1]);

			return NOERROR;
		}
};

#endif // _TestClassC_factory_

