// $Id$
//
//    File: JEventProcessor_TestProcessor.h
// Created: Thu May 21 15:48:40 EDT 2015
// Creator: davidl (on Linux gluon47.jlab.org 2.6.32-358.23.2.el6.x86_64 x86_64)
//

#ifndef _JEventProcessor_TestProcessor_
#define _JEventProcessor_TestProcessor_

#include <JANA/JEventProcessor.h>
using namespace jana;

#include <map>
#include <string>
#include <set>
#include <vector>
using namespace std;

#include "TestClassA.h"
#include "TestClassB.h"
#include "TestClassC.h"
#include "TestClassD.h"

class JEventProcessor_TestProcessor:public jana::JEventProcessor{
	public:
		JEventProcessor_TestProcessor(){}
		~JEventProcessor_TestProcessor(){}
		const char* className(void){return "JEventProcessor_TestProcessor";}

		vector<const TestClassA*> objsA;
		vector<const TestClassB*> objsB;
		vector<const TestClassC*> objsC;
		vector<const TestClassD*> objsD;

		// Associated ancestors maps
		//
		// e.g. associated_all["TestClassB"]["TestClassA"][2][1]
		// indexes the second(=1) associated object of type TestClassA to the
		// third(=2) object of type TestClassB.
		//
		// associated_all["TestClassB"]["TestClassA"].size()
		// is the total number of TestClassB objects while:
		//
		// associated_all["TestClassB"]["TestClassA"][2].size()
		// is the number of associated objects of type TextClassA
		// to the third object of type TestClassB
		map<string, map<string, vector< vector<const JObject*> > > > associated_all;
		map<string, map<string, vector< vector<const JObject*> > > > associated_direct;


		//-------------------------
		// FindSingleObjectAssociations
		//
		// Find associated objects of the template type for the given
		// object.
		template<class T>
		void FindSingleObjectAssociations(const JObject *obj, int max_depth=0){
			vector< vector<const JObject*> > *aobjs = NULL;

			// Get Associated objects by type, but then copy them into
			// container that holds JObject* .
			vector<const T*> aobjst;
			if(max_depth==1){
				obj->Get(aobjst, "", max_depth);  // direct descendents
				aobjs = &associated_direct[obj->className()][T::static_className()];
			}else{
				obj->Get(aobjst); // test default max_depth argument
				aobjs = &associated_all[obj->className()][T::static_className()];
			}

			uint32_t N = aobjs->size();
			aobjs->resize(N+1);
_DBG_<<"["<<obj->className()<<"]["<<T::static_className()<<"] : N="<<N<<" (adding "<<aobjst.size()<<")" << endl;
			for(uint32_t j=0; j<aobjst.size(); j++) (*aobjs)[N].push_back(aobjst[j]);
		}

		//-------------------------
		// FindAssociations
		//
		// Loop over all objects of type T and find associated objects
		// of all types for each.
		template<class T>
		void FindAssociations(vector<const T*> &objs, int max_depth=0){
_DBG_<<"---- "<<objs[0]->className()<<" size="<<objs.size()<<endl;
			for(uint32_t i=0; i<objs.size(); i++){
				FindSingleObjectAssociations<TestClassA>(objs[i], max_depth);
				FindSingleObjectAssociations<TestClassB>(objs[i], max_depth);
				FindSingleObjectAssociations<TestClassC>(objs[i], max_depth);
				FindSingleObjectAssociations<TestClassD>(objs[i], max_depth);
			}
		}

	private:
		jerror_t init(void){return NOERROR;}
		jerror_t brun(jana::JEventLoop *eventLoop, int32_t runnumber){return NOERROR;}
		jerror_t erun(void){return NOERROR;}
		jerror_t fini(void){return NOERROR;}

		jerror_t evnt(jana::JEventLoop *loop, uint64_t eventnumber){
			
			// Get objects
			loop->Get(objsA);
			loop->Get(objsB);
			loop->Get(objsC);
			loop->Get(objsD);
			
			FindAssociations(objsA);
			FindAssociations(objsB);
			FindAssociations(objsC);
			FindAssociations(objsD);

			FindAssociations(objsA,1);
			FindAssociations(objsB,1);
			FindAssociations(objsC,1);
			FindAssociations(objsD,1);

			return NOERROR;
		}
		

};

#endif // _JEventProcessor_TestProcessor_

