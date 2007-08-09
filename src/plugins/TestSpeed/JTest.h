// $Id$
//
//    File: JTest.h
// Created: Wed Aug  8 20:52:23 EDT 2007
// Creator: davidl (on Darwin Amelia.local 8.10.1 i386)
//

#ifndef _JTest_
#define _JTest_

#include <JANA/JObject.h>
#include <JANA/JFactory.h>

class JTest:public JObject{
	public:
		static const char* className(){return "JTest";}
		
		double x;
		double y;
		double z;
};

#endif // _JTest_

