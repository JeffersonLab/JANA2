// $Id$
//
//    File: RawHit.h
// Created: Mon Dec 20 09:16:05 EST 2010
// Creator: davidl (on Darwin eleanor.jlab.org 10.5.0 i386)
//

#ifndef _RawHit_
#define _RawHit_

#include <JANA/JObject.h>
#include <JANA/JFactory.h>

class RawHit:public jana::JObject{
	public:
		JOBJECT_PUBLIC(RawHit);
		
		// Add data members here. For example:
		int crate;
		int slot;
		int channel;
		int adc;
		
		// This method is used primarily for pretty printing
		// the second argument to AddString is printf style format
		void toStrings(vector<pair<string,string> > &items)const{
			AddString(items, "crate", "%d", crate);
			AddString(items, "slot", "%d", slot);
			AddString(items, "channel", "%d", channel);
			AddString(items, "adc", "%d", adc);
		}
		
};

#endif // _RawHit_

