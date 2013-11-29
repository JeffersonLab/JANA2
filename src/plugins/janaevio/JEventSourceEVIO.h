// Author: David Lawrence  July 14, 2006
//
//
// JEventSourceEVIO
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

#include <evioUtil.hxx>
#include <evioFileChannel.hxx>
using namespace evio;

class JEventSourceEVIO:public JEventSource
{
	public:
		JEventSourceEVIO(const char* source_name);
		virtual ~JEventSourceEVIO();		
		virtual const char* className(void){return static_className();}
		static const char* static_className(void){return "JEventSourceEVIO";}

		jerror_t GetEvent(JEvent &event);
		void FreeEvent(JEvent &event);
		jerror_t GetObjects(JEvent &event, JFactory_base *factory);
		
	private:
		evioFileChannel *evioFile;
};

#endif //_JEVENT_SOURCEHDDM_H_
