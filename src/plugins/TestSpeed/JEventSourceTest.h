// Author: David Lawrence  July 14, 2006
//
//
// JEventSourceTest
//
/// Implements JEventSource for HDDM files

#ifndef _JEVENT_SOURCEHDDM_H_
#define _JEVENT_SOURCEHDDM_H_

#include <vector>
#include <string>
using namespace std;

#include <pthread.h>

#include <JANA/JEventSource.h>
#include <JANA/jerror.h>
using namespace jana;


class JEventSourceTest:public JEventSource
{
	public:
		JEventSourceTest(const char* source_name);
		virtual ~JEventSourceTest();		
		virtual const char* className(void){return static_className();}
		static const char* static_className(void){return "JEventSourceTest";}

		jerror_t GetEvent(JEvent &event);
		void FreeEvent(JEvent &event);
		jerror_t GetObjects(JEvent &event, JFactory_base *factory);
		
	private:
	
		unsigned long MAX_IO_RATE_HZ;
};

#endif //_JEVENT_SOURCEHDDM_H_
