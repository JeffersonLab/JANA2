// $Id$
//
//    File: TestClassA_factory.h
// Created: Thu May 21 13:30:10 EDT 2015
// Creator: davidl (on Linux gluon47.jlab.org 2.6.32-358.23.2.el6.x86_64 x86_64)
//

#ifndef _TestClassA_factory_
#define _TestClassA_factory_

#include <JANA/JFactory.h>
#include "TestClassA.h"

class TestClassA_factory:public jana::JFactory<TestClassA>{
	public:
		TestClassA_factory(){};
		~TestClassA_factory(){};


	private:
		jerror_t init(void){return NOERROR;}
		jerror_t brun(jana::JEventLoop *eventLoop, int32_t runnumber){return NOERROR;}
		jerror_t erun(void){return NOERROR;}
		jerror_t fini(void){return NOERROR;}

		jerror_t evnt(jana::JEventLoop *eventLoop, uint64_t eventnumber){

			// Just create 8 objects of type TestClassA
			for(int i=0; i<8; i++){
				_data.push_back(new TestClassA);
			}
			return NOERROR;
		}
};

#endif // _TestClassA_factory_

