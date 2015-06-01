// $Id$
//
//    File: TestClassA.h
// Created: Thu May 21 13:30:10 EDT 2015
// Creator: davidl (on Linux gluon47.jlab.org 2.6.32-358.23.2.el6.x86_64 x86_64)
//

#ifndef _TestClassA_
#define _TestClassA_

#include <JANA/JObject.h>
#include <JANA/JFactory.h>

class TestClassA:public jana::JObject{
	public:
		JOBJECT_PUBLIC(TestClassA);
		
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

#endif // _TestClassA_

