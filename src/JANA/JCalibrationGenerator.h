// $Id$
//
//    File: JCalibrationGenerator.h
// Created: Sat Jan 10 22:59:59 EST 2009
// Creator: davidl (on Darwin Amelia.local 9.6.0 i386)
//

#ifndef _JCalibrationGenerator_
#define _JCalibrationGenerator_

#include <stdint.h>
#include <string>

#include <JANA/jerror.h>

// Place everything in JANA namespace
namespace jana{

class JCalibration;

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

class JCalibrationGenerator{
	public:
		JCalibrationGenerator(){}
		virtual ~JCalibrationGenerator(){}
		
		virtual const char* Description(void)=0; ///< Get string indicating type of calibration this handles
		virtual double CheckOpenable(std::string url, int32_t run, std::string context)=0; ///< Test probability of opening the given calibration
		virtual JCalibration* MakeJCalibration(std::string url, int32_t run, std::string context)=0; ///< Instantiate an JCalibration object (subclass)

};

} // Close JANA namespace

#endif // _JCalibrationGenerator_

