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
using namespace jana;

class JTest:public JObject{
	public:
		JOBJECT_PUBLIC(JTest);
		
		double x;
		double y;
		double z;
		
		void toStrings(vector<pair<string,string> > &items)const{
			AddString(items, "x", "%3.2f", x);
			AddString(items, "y", "%3.2f", y);
			AddString(items, "z", "%3.2f", z);
		}
};

#endif // _JTest_

