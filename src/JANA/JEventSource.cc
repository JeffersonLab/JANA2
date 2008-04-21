// $Id: JEventSource.cc 1347 2005-12-08 16:54:59Z davidl $
//
//    File: JEventSource.cc
// Created: Wed Jun  8 12:31:04 EDT 2005
// Creator: davidl (on Darwin wire129.jlab.org 7.8.0 powerpc)
//

#include <iostream>
#include <iomanip>
using namespace std;

#include "JEventSource.h"
using namespace jana;

//---------------------------------
// JEventSource    (Constructor)
//---------------------------------
JEventSource::JEventSource(const char *source_name)
{
	this->source_name = string(source_name);
	source_is_open = 0;
	pthread_mutex_init(&read_mutex, NULL);
	Nevents_read = 0;
}

//---------------------------------
// ~JEventSource    (Destructor)
//---------------------------------
JEventSource::~JEventSource()
{

}

//----------------
// GetObjects
//----------------
jerror_t JEventSource::GetObjects(JEvent &event, JFactory_base *factory)
{
	/// This only gets called if the subclass doesn't define it. In that
	/// case, the subclass must not support objects.
	return OBJECT_NOT_AVAILABLE;
}

