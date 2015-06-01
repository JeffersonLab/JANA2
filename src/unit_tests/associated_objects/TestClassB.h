// $Id$
//
//    File: TestClassB.h
// Created: Thu May 21 13:30:12 EDT 2015
// Creator: davidl (on Linux gluon47.jlab.org 2.6.32-358.23.2.el6.x86_64 x86_64)
//

#ifndef _TestClassB_
#define _TestClassB_

#include <JANA/JObject.h>
#include <JANA/JFactory.h>

class TestClassB:public jana::JObject{
	public:
		JOBJECT_PUBLIC(TestClassB);
		
		// Add data members here. For example:
		// int id;
		// double E;
		
		// This method is used primarily for pretty printing
		// the second argument to AddString is printf style format
		void toStrings(vector<pair<string,string> > &items)const{
			// AddString(items, "id", "%4d", id);
			// AddString(items, "E", "%f", E);
		}
		
};

#endif // _TestClassB_

