// $Id$
//
//    File: JObject.cc
// Created: Fri Mar 19 09:39:46 EDT 2010
// Creator: davidl (on Darwin harriet.jlab.org 9.8.0 i386)
//

#include "JObject.h"
#include "JFactory_base.h"
#include "JEventLoop.h"
using namespace jana;

// This file is necessary because of the cyclic referencing of
// JObject and JFactory_base. It may be possible to get around
// it using only JObject.h, but this is less tricky.


//-------------------
// GetTag
//-------------------
string JObject::GetTag(void) const
{
	if(factory)return factory->Tag();
	return string("");
}

//------------------
// GetAssociatedDescendants
//------------------
void JObject::GetAssociatedDescendants(JEventLoop *loop, vector<const JObject*> &associatedTo, int max_depth)
{
	/// Find existing objects to which this object is associated to.
	/// It will return any objects that are within max_depth associations.
	///
	/// WARNING: this must build and search the ancestor list of EVERY
	/// object produced by EVERY factory. It is an expensive method
	/// to call. Use it with great caution!
	///
	/// WARNING: this only searches objects that have already been
	/// created. It will not activate factories they may eventually
	/// claim this as an associated object so the list returned may
	/// be incomplete.

	set<const JObject*> associated_set;

	vector<JFactory_base*> factories = loop->GetFactories();
	for(uint32_t i=0; i<factories.size(); i++){
		
		// Do not activate factories that have not yet been activated
		if(!factories[i]->evnt_was_called()) continue;
		
		// Get objects for this factory and search associated objects for each of those
		string classname = className(); // name of this type of object
		vector<void*> vobjs = factories[i]->Get();
		for(uint32_t i=0; i<vobjs.size(); i++){

			JObject *obj = (JObject*)vobjs[i];

			set<const JObject*> already_checked;
			set<const JObject*> objs_found;
			int my_max_depth = max_depth;
			obj->GetAssociatedAncestors(already_checked, my_max_depth, objs_found, classname);

			// Check if we are in list of associated ancestors 
			if(objs_found.find(this) != objs_found.end()) associated_set.insert(obj);
		}
	}
	
	// Copy results into caller's container
	associatedTo.clear();
	set<const JObject*>::iterator it;
	for(it=associated_set.begin(); it!=associated_set.end(); it++){
		associatedTo.push_back(*it);
	}
}
