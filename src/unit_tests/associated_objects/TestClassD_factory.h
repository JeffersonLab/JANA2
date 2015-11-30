// $Id$
//
//    File: TestClassD_factory.h
// Created: Thu May 21 13:30:18 EDT 2015
// Creator: davidl (on Linux gluon47.jlab.org 2.6.32-358.23.2.el6.x86_64 x86_64)
//

#ifndef _TestClassD_factory_
#define _TestClassD_factory_

#include <JANA/JFactory.h>
#include "TestClassD.h"
#include "TestClassC.h"

class TestClassD_factory:public jana::JFactory<TestClassD>{
	public:
		TestClassD_factory(){};
		~TestClassD_factory(){};


	private:
		jerror_t init(void){return NOERROR;}
		jerror_t brun(jana::JEventLoop *eventLoop, int32_t runnumber){return NOERROR;}
		jerror_t erun(void){return NOERROR;}
		jerror_t fini(void){return NOERROR;}

		jerror_t evnt(jana::JEventLoop *loop, uint64_t eventnumber){
		
			vector<const TestClassA*> objsA;
			vector<const TestClassB*> objsB;
			vector<const TestClassC*> objsC;
			loop->Get(objsA);
			loop->Get(objsB);
			loop->Get(objsC);

			// Create objects of type TestClassD for all but the
			// last TestClassC object and associate them.
			for(uint32_t i=0; i<objsC.size()-1; i++){
				
				TestClassD *objD = new TestClassD;
				objD->AddAssociatedObject(objsC[i]);

				_data.push_back(objD);
			}

			// Add one more TestClassC as an associated object
			// to the last TestClassD object
			_data[_data.size()-1]->AddAssociatedObject(objsC[objsC.size()-1]);

			// Also last and next to last TestClassA objects as
			// associated objects to last and next to last TestClassD
			// objects respectively.
			_data[_data.size()-1]->AddAssociatedObject(objsA[objsA.size()-1]);
			_data[_data.size()-2]->AddAssociatedObject(objsA[objsA.size()-2]);

			return NOERROR;
		}
};

#endif // _TestClassD_factory_

