
// Copyright 2020, Jefferson Science Associates, LLC.
// Subject to the terms in the LICENSE file found in the top-level directory.


#include "JGeometryManager.h"

#include <JANA/JLogger.h>
#include <JANA/Compatibility/JGeometryXML.h>
#include <JANA/Compatibility/JGeometryMYSQL.h>


JGeometry *JGeometryManager::GetJGeometry(unsigned int run_number) {

	/// Return a pointer a JGeometry object that is valid for the given run number.
	///
	/// This first searches through the list of existing JGeometry objects created by
	/// this JApplication object to see if it already has the right one. If so, a pointer
	/// to it is returned. If not, a new JGeometry object is created and added to the
	/// internal list. Note that since we need to make sure the list is not modified
	/// by one thread while being searched by another, a mutex is locked while searching
	/// the list. It is <b>NOT</b> efficient to get the JGeometry object pointer every
	/// event. Factories should get a copy in their brun() callback and keep a local
	/// copy of the pointer for use in the evnt() callback.

	// Lock mutex to keep list from being modified while we search it
	std::lock_guard<std::mutex> lock(m_mutex);

	for (auto geometry : geometries) {
		if (geometry->GetRunMin() > (int) run_number) continue;
		if (geometry->GetRunMax() < (int) run_number) continue;
		return geometry;
	}

	// JGeometry object for this run_number doesn't exist in our list.
	// Create a new one and add it to the list.
	// We need to create an object of the appropriate subclass of
	// JGeometry. This is determined by the first several characters
	// of the URL that specifies the calibration database location.
	// For now, only the JGeometryXML subclass exists so we'll
	// just make one of those and defer the heavier algorithm until
	// later.
	const char *url = getenv("JANA_GEOMETRY_URL");
	if (!url) url = "file://./";
	const char *context = getenv("JANA_GEOMETRY_CONTEXT");
	if (!context) context = "default";

	// Decide what type of JGeometry object to create and instantiate it
	string url_str = url;
	string context_str = context;
	JGeometry *g = nullptr;

	if (url_str.find("xmlfile://") == 0 || url_str.find("ccdb://") == 0) {
		g = new JGeometryXML(string(url), run_number, context);
	}
	else if (url_str.find("mysql:") == 0) {
		g = new JGeometryMYSQL(string(url), run_number, context);
	}
	if (g) {
		geometries.push_back(g);
	}
	else {
		jerr << "Cannot make JGeometry object for \"" << url_str << "\" (Don't know how!)" << jendl;
	}
	return g;
}
