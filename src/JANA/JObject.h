// $Id: JObject.h 1709 2006-04-26 20:34:03Z davidl $
//
//    File: JObject.h
// Created: Wed Aug 17 10:57:09 EDT 2005
// Creator: davidl (on Darwin wire129.jlab.org 7.8.0 powerpc)
//

#ifndef _JObject_
#define _JObject_

#include "jerror.h"


typedef unsigned long oid_t;

/// The JObject class is a base class for all data classes.
/// In other words, all classes which are produced by DANA
/// factories.

class JObject{

	public:
	
		JObject(){id = (oid_t)this;}
		JObject( oid_t aId ) : id( aId ) {}

		virtual ~JObject(){}
		
		template<typename T>
		bool IsA(const T *t){return dynamic_cast<const T*>(this)!=0L;}

		oid_t id;
		
};

#endif // _JObject_

