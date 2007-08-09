// Author: David Lawrence  August 8, 2007
//
//

#include <JANA/JApplication.h>

#include "JFactoryGeneratorTest.h"
#include "JTest_factory.h"


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
	
	return NOERROR;
}
