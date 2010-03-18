// $Id$
//
//    File: JRawData.h
// Created: Thu Mar 18 09:45:00 EDT 2010
// Creator: davidl (on Darwin harriet.jlab.org 9.8.0 i386)
//

#ifndef _JRawData_
#define _JRawData_

#include <JANA/JObject.h>
#include <JANA/JFactory.h>
using namespace jana;

class JRawData:public JObject{
	public:
		JOBJECT_PUBLIC(JRawData);
		
		int crate;
		int slot;
		int channel;
		int adc;
		
		void toStrings(vector<pair<string,string> > &items)const{
			AddString(items, "crate", "%d", crate);
			AddString(items, "slot", "%d", slot);
			AddString(items, "channel", "%d", channel);
			AddString(items, "adc", "%d", adc);
		}
};

#endif // _JRawData_

