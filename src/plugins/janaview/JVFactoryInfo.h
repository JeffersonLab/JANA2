// $Id$
//
//    File: JVFactoryInfo.h
// Created: Fri Oct  3 10:36:40 EDT 2014
// Creator: davidl (on Darwin harriet.jlab.org 13.4.0 i386)
//

#ifndef _JVFactoryInfo_
#define _JVFactoryInfo_

#include <string>
using namespace std;

class JVFactoryInfo{
	public:

		string name;
		string tag;
		string nametag;
		bool evnt_called;
		int Nobj;

};

#endif // _JVFactoryInfo_

