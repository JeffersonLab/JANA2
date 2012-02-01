// $Id$
//
//    File: JObject.cc
// Created: Fri Mar 19 09:39:46 EDT 2010
// Creator: davidl (on Darwin harriet.jlab.org 9.8.0 i386)
//

#include "JObject.h"
#include "JFactory_base.h"
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

