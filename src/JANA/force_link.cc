
// This file is an unfortunate work-around for a very annoying feature
// in gcc 4.1 linking behavior. The problem is basically this:
//
// Not all symbols from libJANA.a are included in exectuables linked
// with it. Specifically, I had problems with the jana executable
// not including the JEventSource default destructor which is needed
// by the TestSpeed plugin. This is a problem
// for plugins which expect to get all of the JANA routines through
// the executable. One alternative is to link the plugin with libJANA.a
// but that also causes problems in that multiple jout and jerr
// globals are present which (mysteriously) results in a seg. fault
// near program termination iff a plugin is attached. Another solution
// is to use the -rdynamic flag which causes all GLOBAL symbols to
// automatically be exported. Unfortunately, on CentOS5.3 at least,
// many things (like the JEventSource default destructor) are marked
// as "weak" symbols rather than "global" so they don't get exported
// despite the -rdynamic flag.
//
// So this brings us to this very ugly business. This file exists 
// solely to provide code that forces the symbols to be brought in.
// The force_links routine is called from the JApplication constructor,
// but only when narg<0 which never happens. It is enough though to
// force the linker to include these however.


#include <JANA/JEventSource.h>
#include <JANA/JCalibrationGenerator.h>
#include <JANA/JCalibration.h>
#include <JANA/JEventSink.h>
#include <JANA/JEventSourceGenerator.h>
#include <JANA/JFactory_base.h>
#include <JANA/JFactory.h>
#include <JANA/JFactoryGenerator.h>
#include <JANA/JGeometry.h>
using namespace jana;


class JEventSourceDummy:public JEventSource
{
	public:
		JEventSourceDummy():JEventSource("nada"){}
		jerror_t GetEvent(JEvent&){return NOERROR;}
};

class JEventProcessorDummy:public JEventProcessor
{
	public:
		JEventProcessorDummy(){}
};

class JObjectDummy:public JObject
{
	public:
		JObjectDummy(){GetNameTag();} // GetNameTag call ensures GetTag is emitted
};

class JObjectDummy_factory_tag:public JFactory<JObjectDummy>
{
	public:
		JObjectDummy_factory_tag(){}
		~JObjectDummy_factory_tag(){}
		const char* Tag(void){return "tag";}
};

void force_links(void)
{
	new JEventSourceDummy();
	new JEventProcessorDummy();
	new JObjectDummy_factory_tag();
	new JObjectDummy();
}
