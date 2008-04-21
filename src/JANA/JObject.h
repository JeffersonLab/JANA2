// $Id: JObject.h 1709 2006-04-26 20:34:03Z davidl $
//
//    File: JObject.h
// Created: Wed Aug 17 10:57:09 EDT 2005
// Creator: davidl (on Darwin wire129.jlab.org 7.8.0 powerpc)
//

#ifndef _JObject_
#define _JObject_

#include <cassert>
#include <map>
#include <vector>
#include <string>
using std::pair;
using std::map;
using std::vector;
using std::string;


/// The JObject class is a base class for all data classes.
/// (See JFactory and JFactory_base for algorithm classes.)
///
///
/// The following line should be included in the public definition of all
/// classes which inherit from JObject with the argument being the name of the
/// class without any quotes.
/// e.g. for a class named "MyClass" :
/// 
///  public:
///     JOBJECT_PUBLIC(MyClass);
///
/// This will define a virtual method <i>className()</i> and a static
/// method <i>static_className()</i> that are used by JANA to identify
/// the object's last generation in the inheritance chain by name. This
/// also allows for possible upgrades to JANA in the future without
/// requiring classes that inherit from JObject to be redefined explicity.
#define JOBJECT_PUBLIC(T) \
	virtual const char* className(void) const {return static_className();} \
	static const char* static_className(void) {return #T;}

// Place everything in JANA namespace
namespace jana
{

class JObject{

	public:
		JOBJECT_PUBLIC(JObject);

		typedef unsigned long oid_t;
	
		JObject(){id = (oid_t)this;}
		JObject( oid_t aId ) : id( aId ) {}

		virtual ~JObject(){}
		
		/// Test if this object is of type T by checking its className() against T::static_className()
		template<typename T> bool IsA(const T *t) const {return dynamic_cast<const T*>(this)!=0L;}

		/// Test if this object is of type T (or a descendant) by attempting a dynamic cast
		template<typename T> bool IsAT(const T *t) const {return dynamic_cast<const T*>(this)!=0L;}

		// Methods for handling associated objects
		inline void AddAssociatedObject(const JObject *obj);
		inline void RemoveAssociatedObject(const JObject *obj);
		template<typename T> void Get(vector<const T*> &ptrs, string classname="") const ;
		template<typename T> void GetT(vector<const T*> &ptrs) const ;

		// Methods for handling pretty formatting for dumping to the screen or file
		virtual void toStrings(vector<pair<string,string> > &items)const;
		template<typename T> void AddString(vector<pair<string,string> > &items, const char *name, const char *format, const T &val) const;

		oid_t id;
	
	private:
		
		map<const JObject*, string> associated;
		
};

//--------------------------
// AddAssociatedObject
//--------------------------
void JObject::AddAssociatedObject(const JObject *obj)
{
	/// Add a JObject to the list of associated objects

	assert(obj!=NULL);
	
	associated[obj] = obj->className();
}

//--------------------------
// RemoveAssociatedObject
//--------------------------
void JObject::RemoveAssociatedObject(const JObject *obj)
{
	/// Remove the specified JObject from the list of associated
	/// objects.

	map<const JObject*, string>::iterator iter = associated.find(obj);
	
	if(iter!=associated.end()){
		associated.erase(iter);
	}
}

//--------------------------
// Get
//--------------------------
template<typename T>
void JObject::Get(vector<const T*> &ptrs, string classname) const
{
	/// Fill the given vector with pointers to the associated 
	/// JObjects of the type on which the vector is based. The
	/// objects are chosen by matching their class names 
	/// (obtained via JObject::className()) either
	/// to the one provided in classname or to T::static_className()
	/// if classname is an empty string.
	///
	/// The contents of ptrs are cleared upon entry.
	
	ptrs.clear();
	
	if(classname=="")classname=T::static_className();
	
	map<const JObject*, string>::iterator iter = associated.begin();
	for(; iter!=associated.end(); iter++){
		if(iter->second == classname)ptrs.push_back(iter->first);
	}	
}

//--------------------------
// GetT
//--------------------------
template<typename T>
void JObject::GetT(vector<const T*> &ptrs) const
{
	/// Fill the given vector with pointers to the associated 
	/// JObjects of the type on which the vector is based. This is
	/// similar to the Get() method except objects are selected
	/// by attempting a dynamic_cast to type const T*. This allows
	/// one to select a list of all objects who have a type T
	/// somewhere in their inheritance chain.
	///
	/// A potential issue with this method is that the dynamic_cast
	/// does not always work correctly for objects created via a
	/// plugin when the cast occurs outside of the plugin or
	/// vice versa.
	///
	/// The contents of ptrs are cleared upon entry.
	
	ptrs.clear();
	
	string classname=T::static_className();
	
	map<const JObject*, string>::iterator iter = associated.begin();
	for(; iter!=associated.end(); iter++){
		const T *ptr = dynamic_cast<const T*>(iter->first);
		if(ptr != NULL)ptrs.push_back(ptr);
	}	
}

//--------------------------
// toStrings
//--------------------------
inline void JObject::toStrings(vector<pair<string,string> > &items) const
{
	/// Fill the given "items" vector with items representing the (important)
	/// data members of this object. The structure of "items" is a vector
	/// of pairs. The "first" element of the pair is the name of the item
	/// as it should be displayed when dumping the item to the screen. For
	/// example, one may wish to include units using a string like "r (cm)".
	/// The "second" element of the pair is a formatted string containing the
	/// value as it should be displayed.
	///
	/// To facilitate this, the AddString() method exists which allows
	/// items to be added with the desired formatting using a single line.
	///
	/// This is a virtual method that is expected (but not required)
	/// to be implemented by all classes that inherit from JObject.

	AddString(items, "JObject", "0x%08x", (unsigned long)this);
}

//--------------------------
// AddString
//--------------------------
template<typename T>
void JObject::AddString(vector<pair<string,string> > &items, const char *name, const char *format, const T &val) const
{
	/// Write the given value (val) to a string using the sprintf style formatting
	/// string (format) and add it to the given vector (items) with the column
	/// name "name". This is intended for use in the toStrings() method of
	/// classes that inherit from JObject.
	char str[256];
	sprintf(str, format, val);
	pair<string, string> item;
	item.first = name;
	item.second = string(str);
	items.push_back(item);
}


} // Close JANA namespace

#endif // _JObject_

