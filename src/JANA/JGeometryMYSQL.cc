// $Id$
//
//    File: JGeometryMYSQL.cc
// Created: Wed May  7 16:21:37 EDT 2008
// Creator: davidl (on Darwin swire-d95.jlab.org 8.11.1 i386)
//

#include "JGeometryMYSQL.h"
using namespace jana;

//---------------------------------
// JGeometryMYSQL    (Constructor)
//---------------------------------
JGeometryMYSQL::JGeometryMYSQL(string url, int run, string context):JGeometry(url,run,context)
{

}

//---------------------------------
// ~JGeometryMYSQL    (Destructor)
//---------------------------------
JGeometryMYSQL::~JGeometryMYSQL()
{

}

//---------------------------------
// Get
//---------------------------------
bool JGeometryMYSQL::Get(string path, string &sval)
{

	// Looks like we failed to find the requested item. Let the caller know.
	return false;
}

//---------------------------------
// Get
//---------------------------------
bool JGeometryMYSQL::Get(string path, map<string, string> &svals)
{
	// Looks like we failed to find the requested item. Let the caller know.
	return false;
}

//---------------------------------
// GetMultiple
//---------------------------------
bool JGeometryMYSQL::GetMultiple(string xpath, vector<string> &vsval)
{
	return false;
}

//---------------------------------
// GetMultiple
//---------------------------------
bool JGeometryMYSQL::GetMultiple(string xpath, vector<map<string, string> >&vsvals)
{
	return false;
}

//---------------------------------
// GetXPaths
//---------------------------------
void JGeometryMYSQL::GetXPaths(vector<string> &paths, ATTR_LEVEL_t level, const string &filter)
{

}
