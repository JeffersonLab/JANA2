// $Id$
//
//    File: JEventSourceGenerator.h
// Created: Tue Jun 27 06:56:19 EDT 2006
// Creator: davidl (on Darwin Harriet.local 8.6.0 powerpc)
//

#ifndef _JEventSourceGenerator_
#define _JEventSourceGenerator_

#include "jerror.h"

namespace jana{
class JApplication;
class JEventSource;
} // Close JANA namespace

extern "C" {
extern void InitPlugin(jana::JApplication *app);
}

// Place everything in JANA namespace
namespace jana{


/// This is a base class for all event source generators. JANA implements
/// event sources in a modular way so that new types of sources can be
/// easily added. Typically, this just means different file formats, but
/// an also be networked or shared memory sources. To provide a new
/// source type, a class must inherit from JEventSourceGenerator and
/// implement the three virtual methods it defines:
///
/// Description:
/// 	This method should just return a short string that can be used to
/// 	inform the user what type of source this implements. It is purely
/// 	informational in that the framework never looks at its contents.
///
/// CheckOpenable:
/// 	This method should "peek" at the source to see if it is one that
/// 	it can open. It should return a value between 0 and 1 inclusive
/// 	with 0 meaning "cannot open" and 1 meaning "absolutely can open".
/// 	The check can be as simple as looking at the source name (e.g.
/// 	does it have a specific suffix) or more involved (e.g. opening
/// 	the file and checking for a magic header). Note that it should
/// 	not be assumed that the source represents a file name. It could
/// 	indicate a URL or how to connect to some other non-file source.
///
/// MakeJEventSource:
/// 	This will be called when the value of CheckOpenable returned
/// 	by this object is larger than that returned by all other
/// 	JEventSourceGenerator objects. This should simply instantiate
/// 	an object of the JEventSource based class that does the actual
/// 	work of reading objects from the source.

class JEventSourceGenerator{
	public:
		JEventSourceGenerator(){}; 
		virtual ~JEventSourceGenerator(){};
		virtual const char* className(void){return static_className();}
		static const char* static_className(void){return "JEventSourceGenerator";}
		
		virtual const char* Description(void)=0; ///< Get string indicating type of source this handles
		virtual double CheckOpenable(string source)=0; ///< Test probability of opening the given source
		virtual JEventSource* MakeJEventSource(string source)=0; ///< Instantiate an JEventSource object (subclass)
		
	protected:
	
	
	private:

};

} // Close JANA namespace

#endif // _JEventSourceGenerator_

