// Author: David Lawrence  August 8, 2007
//
//

#include <JANA/JApplication.h>

#include "JFactoryGeneratorTest.h"
#include "JTest_factory.h"
#include "JRawData.h"

// The JFactory_RawData factory doesn't actually contain code and only
// serves as a container for objects generated in JEventSourceTest
typedef JFactory<JRawData> JFactory_RawData;

//---------------------------------
// JFactoryGeneratorTest    (Constructor)
//---------------------------------
JFactoryGeneratorTest::JFactoryGeneratorTest()
{

}

//---------------------------------
// ~JFactoryGeneratorTest    (Destructor)
//---------------------------------
JFactoryGeneratorTest::~JFactoryGeneratorTest()
{

}

//---------------------------------
// GenerateFactories
//---------------------------------
jerror_t JFactoryGeneratorTest::GenerateFactories(JEventLoop *loop)
{
	loop->AddFactory(new JTest_factory());
	loop->AddFactory(new JFactory_RawData());
	
	return NOERROR;
}
