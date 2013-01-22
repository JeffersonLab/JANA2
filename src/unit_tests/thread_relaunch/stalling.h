// $Id$
//
//    File: stalling.h
// Created: Tue Jan 22 09:41:30 EST 2013
// Creator: davidl (on Darwin harriet.jlab.org 11.4.2 i386)
//

#ifndef _stalling_
#define _stalling_

#include <JANA/JObject.h>
#include <JANA/JFactory.h>

class stalling:public jana::JObject{
	public:
		JOBJECT_PUBLIC(stalling);
		
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

#endif // _stalling_

