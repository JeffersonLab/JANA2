// $Id$
//
//    File: JFactoryGenerator.h
// Created: Tue Jun 27 11:19:21 EDT 2006
// Creator: davidl (on Darwin swire-b241.jlab.org 8.6.0 powerpc)
//

#ifndef _JFactoryGenerator_
#define _JFactoryGenerator_

#include "jerror.h"

// Place everything in JANA namespace
namespace jana{

class JEventLoop;

/// This class is used by the JEventLoop objects to create factories
/// in each event processing thread. One factory generator may be used
/// to generate any number of factories. This is one of the key places
/// where one specializes JANA for a specific project.
///
/// One can also register a JFactoryGenerator from a plugin. This
/// is how one could add factories or "override" compiled in factories
/// in any existing JANA program.
///
/// This is a virtual class so to use it, one must inherit from it
/// and register an instance of the subclass with the JApplication
/// object like this:
///
/// <pre>
///	JFactoryGenerator *gen = new MyFactoryGenerator();
/// 	japp->AddFactoryGenerator(gen);
/// </pre>
///
/// The derived class needs to implement two methods:
/// <ul>
/// 	<li>Description:
///	This provides a brief description of the generator. It is not
///	currently used anywhere, but may eventually be used to provide
///	the end user with some information about the generators
/// 	being used. If it is not defined, a default description of
///	"Not available." will be used
///
///	<li>GenerateFactories:
///	This gets called by each event loop before event processing
///	begins. Thus, it will get called once for every processing thread.
///	It gets a pointer to the JEventLoop passed as its only argument
///	when called. The AddFactory(...) method of JEventLoop should then be
///	called to add a factory.
/// </ul>

class JFactoryGenerator{
	public:
		JFactoryGenerator(){}
		virtual ~JFactoryGenerator(){}
		virtual const char* className(void){return static_className();}
		static const char* static_className(void){return "JFactoryGenerator";}
		
		virtual const char* Description(void){return "Not available.";}
		virtual jerror_t GenerateFactories(JEventLoop *loop)=0;
		
	protected:
	
	
	private:

};

} // Close JANA namespace

#endif // _JFactoryGenerator_

