// $Id$
//
//    File: jana_test.h
// Created: Mon Oct 23 22:39:14 EDT 2017
// Creator: davidl (on Darwin harriet 15.6.0 i386)
//

#ifndef _jana_test_
#define _jana_test_

#include <vector>
#include <utility>
#include <string>

#include <JANA/JObject.h>
#include <JANA/JFactory.h>

class jana_test:public JObject{
	public:
		JOBJECT_PUBLIC(jana_test);
		
		// Add data members here. For example:
		// int id;
		// double E;

};

#endif // _jana_test_

