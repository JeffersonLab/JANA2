// Author: David Lawrence  August 8, 2007
//
//

#ifndef _JFactoryGeneratorTest_
#define _JFactoryGeneratorTest_

#include "JANA/jerror.h"
#include "JANA/JFactoryGenerator.h"
using namespace jana;

class JFactoryGeneratorTest: public JFactoryGenerator{
	public:
		JFactoryGeneratorTest();
		virtual ~JFactoryGeneratorTest();
		virtual const char* className(void){return static_className();}
		static const char* static_className(void){return "JFactoryGeneratorTest";}
		
		jerror_t GenerateFactories(JEventLoop*);

	protected:
	
	
	private:

};

#endif // _JFactoryGeneratorTest_

