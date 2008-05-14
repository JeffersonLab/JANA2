// $Id$
//
//    File: JGeometryMYSQL.h
// Created: Wed May  7 16:21:37 EDT 2008
// Creator: davidl (on Darwin swire-d95.jlab.org 8.11.1 i386)
//

#ifndef _JGeometryMYSQL_
#define _JGeometryMYSQL_

#include <JANA/jerror.h>
#include <JANA/JGeometry.h>
#include <JANA/jana_config.h>

// Place everything in JANA namespace
namespace jana
{

class JGeometryMYSQL:public JGeometry{
	public:
		JGeometryMYSQL(string url, int run, string context="default");
		virtual ~JGeometryMYSQL();
		
		bool Get(string xpath, string &sval);
		bool Get(string xpath, map<string, string> &svals);
		void GetXPaths(vector<string> &xpaths, ATTR_LEVEL_t level, const string &filter="");

	protected:
	
	
	private:
		JGeometryMYSQL();

};

}

#endif // _JGeometryMYSQL_

