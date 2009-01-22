// $Id$
//
//    File: JCalibration.cc
// Created: Fri Jul  6 16:24:24 EDT 2007
// Creator: davidl (on Darwin fwing-dhcp61.jlab.org 8.10.1 i386)
//

#include "JCalibration.h"
using namespace jana;

//---------------------------------
// JCalibration    (Constructor)
//---------------------------------
JCalibration::JCalibration(string url, int run, string context)
{
	this->url = url;
	this->run_requested = run;
	this->context = context;
}

//---------------------------------
// ~JCalibration    (Destructor)
//---------------------------------
JCalibration::~JCalibration()
{
	// Here we need to delete any data being kept in the "stored" map.
	// This is difficult since the pointers are kept as void* types
	// so we can't call delete without type casting them back into the
	// appropriate pointer type. Note that everywhere but here, the
	// stored vector is accessed through a templated method so the caller
	// provides the type information.
	//
	// The best we can do here is to check the typeid name against the list
	// of containers based on primitive types (+string) to see if we
	// can match it that way. We do this by calling the TryDelete templated
	// method which will build each of the 4 container types based on
	// the primitive type used for the template specialization parameter.
	
	// Loop over stored data containers
	map<pair<string,string>, void*>::iterator iter;
	for(iter=stored.begin(); iter!=stored.end(); iter++){
	
				if(TryDelete<         double >(iter));
		else	if(TryDelete<         float  >(iter));
		else	if(TryDelete<         int    >(iter));
		else	if(TryDelete<         long   >(iter));
		else	if(TryDelete<         short  >(iter));
		else	if(TryDelete<         char   >(iter));
		else	if(TryDelete<unsigned int    >(iter));
		else	if(TryDelete<unsigned long   >(iter));
		else	if(TryDelete<unsigned short  >(iter));
		else	if(TryDelete<unsigned char   >(iter));
		else	if(TryDelete<         string >(iter));
		else{
			_DBG_<<"Unable to delete calibration constants of type: "<<iter->first.second<<std::endl;
			_DBG_<<"namepath: "<<iter->first.first<<std::endl;
			_DBG_<<std::endl;
		}
	}
}

//---------------------------------
// RecordRequest
//---------------------------------
void JCalibration::RecordRequest(string namepath, string type_name)
{
	/// Record a request for a set of calibration constants. 
	map<string, vector<string> >::iterator iter = accesses.find(namepath);
	if(iter==accesses.end()){
		vector<string> types;
		types.push_back(type_name);
		accesses[namepath] = types;
	}else{
		iter->second.push_back(type_name);
	}
}

