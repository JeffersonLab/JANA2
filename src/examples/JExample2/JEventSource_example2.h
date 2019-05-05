// $Id$
//
//    File: JEventxample2.cc
// Created: Thu Apr 26 22:07:34 EDT 2018
// Creator: davidl (on Darwin amelia.jlab.org 17.5.0 x86_64)
//

#include <memory>
#include <utility>
#include <random>


#include <JANA/JApplication.h>
#include <JANA/JEventSourceGenerator.h>
#include <JANA/JEventSource.h>
#include <JANA/JEventSourceManager.h>
#include <JANA/JQueueSimple.h>
#include <JANA/JEvent.h>
#include <JANA/JEventSource.h>

#include "MyHit.h"


//-------------------------------------------------------------------------
// This class would be responsible for opening and reading from the source
// of events (e.g. a file or network socket). It should read an event every
// time the GetEvent() method is called. The actual data read should be
// encapsulated in the form of a JEvent object.
class JEventSource_example2: public JEventSource{
	public:

		// Constructor must take string and JApplication pointer as arguments
		JEventSource_example2(std::string source_name, JApplication *app):JEventSource(source_name, app){}
		virtual ~JEventSource_example2(){}

		// A description of this source type must be provided as a static member
		static std::string GetDescription(void){ return "Event source for JExample2"; }

		// See JEventSource_example2.cc for details on these
		void Open(void);
		bool GetObjects(const std::shared_ptr<const JEvent>& aEvent, JFactory* aFactory);
		void GetEvent(std::shared_ptr<JEvent>);
};

