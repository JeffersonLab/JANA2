// $Id: JObject.h 1709 2006-04-26 20:34:03Z davidl $
//
//    File: JObject.h
// Created: Wed Aug 17 10:57:09 EDT 2005
// Creator: davidl (on Darwin wire129.jlab.org 7.8.0 powerpc)
//

#ifndef _JObject_
#define _JObject_

#include <cstdio>
#include <sstream>
#include <cassert>
#include <map>
#include <vector>
#include <set>
#include <string>
#include <typeinfo>
#include <stdint.h>
using std::pair;
using std::map;
using std::set;
using std::vector;
using std::string;
using std::stringstream;

// The following is here just so we can use ROOT's THtml class to generate documentation.
#include "cint.h"


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
namespace jana{

class JFactory_base;
class JEventLoop;

class JObject{

	public:
		JOBJECT_PUBLIC(JObject);

		typedef unsigned long long oid_t;
	
		JObject() : id((oid_t)this),append_types(false),factory(NULL) {}
		JObject( oid_t aId ) : id( aId ),append_types(false),factory(NULL) {}

		virtual ~JObject(){
			for(unsigned int i=0; i<auto_delete.size(); i++)delete auto_delete[i];
		}
		
		/// Test if this object is of type T by checking its className() against T::static_className()
		template<typename T> bool IsA(const T *t) const {return dynamic_cast<const T*>(this)!=0L;}

		/// Test if this object is of type T (or a descendant) by attempting a dynamic cast
		template<typename T> bool IsAT(const T *t) const {return dynamic_cast<const T*>(this)!=0L;}

		// Methods for handling associated objects
		inline void AddAssociatedObject(const JObject *obj);
		inline void AddAssociatedObjectAutoDelete(JObject *obj, bool auto_delete=true);
		inline void RemoveAssociatedObject(const JObject *obj);
		inline void ClearAssociatedObjects(void);
		inline bool IsAssociated(const JObject* locObject) const {return (associated.find(locObject) != associated.end());}
		template<typename T> void Get(vector<const T*> &ptrs, string classname="", int max_depth=1000000) const ;
		template<typename T> void GetT(vector<const T*> &ptrs) const ;
		template<typename T> void GetSingle(const T* &ptrs, string classname="") const ;
		template<typename T> void GetSingleT(const T* &ptrs) const ;
		template<typename T> void GetAssociatedAncestors(set<const JObject*> &already_checked, int &max_depth, set<const T*> &objs_found, string classname="") const;
		template<typename T> void GetAssociatedDescendants(JEventLoop *loop, vector<const T*> &associatedTo, int max_depth=1000000);
		void GetAssociatedDescendants(JEventLoop *loop, vector<const JObject*> &associatedTo, int max_depth=1000000);

		template<typename T,typename S> void CopyToVector(T itbegin, T itend, vector<const S*> &v) const;

		// Methods for handling pretty formatting for dumping to the screen or file
		virtual void toStrings(vector<pair<string,string> > &items)const;
		template<typename T> void AddString(vector<pair<string,string> > &items, const char *name, const char *format, const T &val) const;

		// Methods for attaching and retrieving log messages to/from object
		void AddLog(string &message) const {messagelog.push_back(message);}
		void AddLog(vector<string> &messages) const {messagelog.insert(messagelog.end(), messages.begin(), messages.end());}
		void GetLog(vector<string> &messagelog) const {messagelog = this->messagelog;}

		// Misc methods
		bool GetAppendTypes(void) const {return append_types;} ///< Get state of append_types flag (for AddString)
		void SetAppendTypes(bool append_types){this->append_types=append_types;} ///< Set state of append_types flag (for AddString)
		void SetFactoryPointer(JFactory_base *factory){this->factory=factory;}
		JFactory_base * GetFactoryPointer(void) const {return factory;}
		string GetName(void) const {return string(className());}
		string GetTag(void) const ;
		string GetNameTag(void) const {return GetName() + (GetTag()=="" ? "":":") + GetTag();}

		oid_t id;
	
	private:
		
		bool append_types;
		set<const JObject*> associated;
		// map<const JObject*, string> associated; replaced with set in jana 0.7.7
		vector<JObject*> auto_delete;
		mutable vector<string> messagelog;
		JFactory_base *factory;
		
};

#if !defined(__CINT__) && !defined(__CLING__)


//--------------------------
// AddAssociatedObject
//--------------------------
void JObject::AddAssociatedObject(const JObject *obj)
{
	/// Add a JObject to the list of associated objects

	assert(obj!=NULL);
	
	associated.insert(obj);
	//associated[obj] = obj->className();
}

//--------------------------
// AddAssociatedObjectAutoDelete
//--------------------------
void JObject::AddAssociatedObjectAutoDelete(JObject *obj, bool auto_delete)
{
	/// Add a JObject to the list of associated objects. If the auto_delete
	/// flag is true, then automatically delete it when this object is
	/// deleted. Otherwise, this behaves identically to the AddAssociatedObject
	/// method.
	///
	/// Note that if the object is removed via RemoveAssociatedObject(...)
	/// then the object is NOT deleted. BUT, if the entire list of associated
	/// objects is cleared via ClearAssociatedObjects, then the object will
	/// be deleted.

	AddAssociatedObject(obj);
	
	if(auto_delete)this->auto_delete.push_back(obj);
}

//--------------------------
// RemoveAssociatedObject
//--------------------------
void JObject::RemoveAssociatedObject(const JObject *obj)
{
	/// Remove the specified JObject from the list of associated
	/// objects. This will NOT delete the object even if the
	/// object was added with the AddAssociatedObjectAutoDelete(...)
	/// method with the auto_delete flag set.

	// map<const JObject*, string>::iterator iter = associated.find(obj);
	auto iter = associated.find(obj);
	
	if(iter!=associated.end()){
		associated.erase(iter);
	}
}
	
//--------------------------
// ClearAssociatedObjects
//--------------------------
void JObject::ClearAssociatedObjects(void)
{
	/// Remove all associated objects from the associated objects list.
	/// This will also delete any objects that were added via the
	/// AddAssociatedObjectAutoDelete(...) method with the auto_delete
	/// flag set.
	
	// Clear pointers to associated objects
	associated.clear();
	
	// Delete objects in the auto_delete list
	for(unsigned int i=0; i<auto_delete.size(); i++)delete auto_delete[i];
	auto_delete.clear();
}

//--------------------------
// CopyToVector
//
// This litte utility method is here because the g++ 4.4.7
// compiler was complaining about the declaration of 
// set<const T*>::iterator it; in the Get method. Very strange
// but this at least avoids ever having to declare the variable.
//--------------------------
template<typename T,typename S>
void JObject::CopyToVector(T itbegin, T itend, vector<const S*> &v) const
{
	for(T it=itbegin; it!=itend; it++) v.push_back(*it);
}

//--------------------------
// Get
//--------------------------
template<typename T>
void JObject::Get(vector<const T*> &ptrs, string classname, int max_depth) const
{
	/// Fill the given vector with pointers to the associated objects of the
	/// type on which the vector is based. The objects are chosen by matching
	/// their class names (obtained via JObject::className()) either to the
	/// one provided in classname or to T::static_className() if classname is
	/// an empty string. Associations will be searched to a level of max_depth
	/// to find all objects of the requested type. By default, max_depth is
	/// set to a very large number so that all associations are found. To 
	/// limit the search to only objects directly associated with this one,
	/// set max_depth to either "0" or "1".
	///
	/// The contents of ptrs are cleared upon entry.

	if(classname=="")classname=T::static_className();
	
	// Use the GetAssociatedAncestors method which may call itself
	// recursively to search all levels of association (at or
	// below this object. Objects for which this is an associated
	// object are not checked for).
	set<const JObject*> already_checked;
	set<const T*> objs_found;
	int my_max_depth = max_depth;
	GetAssociatedAncestors(already_checked, my_max_depth, objs_found, classname);
	
	// Copy results into caller's container
	ptrs.clear();
	CopyToVector(objs_found.begin(), objs_found.end(), ptrs);
//	set<const T*>::iterator it;
//	for(it=objs_found.begin(); it!=objs_found.end(); it++){
//		ptrs.push_back(*it);
//	}	
}

//--------------------------
// GetAssociatedAncestors
//--------------------------
template<typename T>
void JObject::GetAssociatedAncestors(set<const JObject*> &already_checked, int &max_depth, set<const T*> &objs_found, string classname) const
{
	/// Get associated objects of the specified type (either "T" or classname).
	/// Check also for associated objects of any associated objects
	/// to a level of max_depth associations. This method calls itself
	/// recursively so care is taken to only check the associated objects
	/// of each object encountered only once.
	///
	/// The "already_checked" parameter should be passed in as an empty container
	/// that is used to keep track of which objects had their direct associations
	/// checked. "max_depth" indicates the maximum level of associations to check
	/// (n.b. both "0" and "1" means only check direct associations.) This must
	/// be passed as a reference to an existing int since it is modified in order
	/// to keep track of the current depth in the recursive calls. Set max_depth
	/// to a very high number (like 1000000) to check all associations. The
	/// "objs_found" container will contain the actual associated objects found.
	/// The objects are chosen by matching their class names (obtained via 
	/// JObject::className()) either to the one provided in "classname" or to
	/// T::static_className() if classname is an empty string.

	if(already_checked.find(this) == already_checked.end()) already_checked.insert(this);

	if(classname=="")classname=T::static_className();
	max_depth--;
	
	//map<const JObject*, string>::const_iterator iter = associated.begin();
	for( auto obj : associated ){
	
		// Add to list if appropriate
		if( classname == obj->className() ){
			objs_found.insert( dynamic_cast<const T*>(obj) );
		}

		// Check this object's associated objects if appropriate
		if(max_depth<=0) continue;
		if(already_checked.find(obj) != already_checked.end()) continue;
		already_checked.insert(obj);
		obj->GetAssociatedAncestors(already_checked, max_depth, objs_found, classname);
	}	

	max_depth++;
}

//--------------------------
// GetAssociatedDescendants
//--------------------------
template<typename T>
void GetAssociatedDescendants(JEventLoop *loop, vector<const T*> &associatedTo, int max_depth=1000000)
{
	/// Find objects of type "T" for which this object appears in its
	/// associated ancestors list. (This is kind of the opposite of
	/// the "Get()" method.)
	///
	/// WARNING: this must build and search the ancestor list of EVERY
	/// object produced by EVERY factory. It is an expensive method
	/// to call. Use it with great caution!
	///
	/// WARNING: this only searches objects that have already been
	/// created. It will not activate factories they may eventually
	/// claim this as an associated object so the list returned may
	/// be incomplete.
	///
	/// WARNING: this templated method works by first calling the 
	/// JObject form and then dynamically casting each of those to see
	/// if they of type "T". This makes this an even more expensive
	/// call. Again, use with great caution!!
	
	vector<const JObject*> ajobjs;
	GetAssociatedDescendants(loop, ajobjs, max_depth);
	for(uint32_t i=0; i<ajobjs.size(); i++){
		const T *ptr = dynamic_cast<const T*>(ajobjs[i]);
		if(ptr != NULL) associatedTo.push_back(ptr);
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
	
	//map<const JObject*, string>::const_iterator iter = associated.begin();
	//for(; iter!=associated.end(); iter++){
	for( auto obj : associated ){
		const T *ptr = dynamic_cast<const T*>(obj);
		if(ptr != NULL)ptrs.push_back(ptr);
	}	
}

//-------------
// GetSingle
//-------------
template<class T>
void JObject::GetSingle(const T* &t, string classname) const
{
	/// This is a convenience method that can be used to get a pointer to the single
	/// associate object of type T.
	///
	/// The objects are chosen by matching their class names 
	/// (obtained via JObject::className()) either
	/// to the one provided in classname or to T::static_className()
	/// if classname is an empty string.
	///
	/// If no object of the specified type is found, a NULL pointer is
	/// returned.

	t = NULL;

	if(classname=="")classname=T::static_className();
	
	//map<const JObject*, string>::const_iterator iter = associated.begin();
	//for(; iter!=associated.end(); iter++){
	for( auto obj : associated ){
		if( classname == obj->className() ){
			t = dynamic_cast<const T*>(obj);
			if(t!=NULL)return;
		}
	}	
}

//-------------
// GetSingleT
//-------------
template<class T>
void JObject::GetSingleT(const T* &t) const
{
	/// This is a convenience method that can be used to get a pointer to the single
	/// associate object of type T.
	///
	/// This is similar to the GetSingle() method except objects are selected
	/// by attempting a dynamic_cast to type const T*. This allows
	/// one to select a list of all objects who have a type T
	/// somewhere in their inheritance chain.
	/// The objects are chosen by matching their class names 
	/// (obtained via JObject::className()) either
	/// to the one provided in classname or to T::static_className()
	/// if classname is an empty string.
	///
	/// If no object of the specified type is found, a NULL pointer is
	/// returned.
	
	t = NULL;

	//map<const JObject*, string>::const_iterator iter = associated.begin();
	//for(; iter!=associated.end(); iter++){
	for( auto obj : associated ){
		t = dynamic_cast<const T*>(obj);
		if(t!=NULL)return;
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
	///
	/// The append_type flag provides a facility for recording the data type
	/// and value with default formatting into items. This can be used 
	/// by a generic convertor (not part of JANA) to auto-generate a
	/// representation of this object for use in some other persistence
	/// package (e.g. ROOT files).
	///
	/// If the append_types flag is set then the data type of "val" is
	/// automatically appended with a colon (:) separator to the
	/// name (first) part of the pair. In addition, "val" is converted
	/// using stringstream and appended as well, also with a colon (:)
	/// separator. For example, if the value of name passed in is "px"
	/// and T is of type double, then the first member of the pair
	/// appended to items will be something like "px:double:1.23784"
	/// which can be decifered later to get the name, type, and value
	/// of the data member.
	///
	/// By default, the append_types flag is not set and the name part
	/// of the pair is a straight copy of the name argument that is
	/// passed in.

	char str[256];
	sprintf(str, format, val);

	stringstream ss;
	ss<<name;
	if(append_types){
		if(typeid(T)==typeid(int)){
			ss<<":int:"<<val;
		}else if(typeid(T)==typeid(int32_t)){
			ss<<":int:"<<val;
		}else if(typeid(T)==typeid(unsigned int)){
			ss<<":uint:"<<val;
		}else if(typeid(T)==typeid(uint32_t)){
			ss<<":uint:"<<val;
		}else if(typeid(T)==typeid(long)){
			ss<<":long:"<<val;
		}else if(typeid(T)==typeid(int64_t)){
			ss<<":long:"<<val;
		}else if(typeid(T)==typeid(unsigned long)){
			ss<<":ulong:"<<val;
		}else if(typeid(T)==typeid(uint64_t)){
			ss<<":ulong:"<<val;
		}else if(typeid(T)==typeid(short)){
			ss<<":short:"<<val;
		}else if(typeid(T)==typeid(int16_t)){
			ss<<":short:"<<val;
		}else if(typeid(T)==typeid(unsigned short)){
			ss<<":ushort:"<<val;
		}else if(typeid(T)==typeid(uint16_t)){
			ss<<":ushort:"<<val;
		}else if(typeid(T)==typeid(float)){
			ss<<":float:"<<val;
		}else if(typeid(T)==typeid(double)){
			ss<<":double:"<<val;
		}else if(typeid(T)==typeid(string)){
			ss<<":string:"<<val;
		}else if(typeid(T)==typeid(const char*)){
			ss<<":string:"<<val;
		}else if(typeid(T)==typeid(char*)){
			ss<<":string:"<<val;
		}else{
			ss<<":unknown:"<<str;
		}
	}

	pair<string, string> item;
	item.first = ss.str();
	item.second = string(str);
	items.push_back(item);
}

#endif // __CINT__  __CLING__


} // Close JANA namespace

#endif // _JObject_

