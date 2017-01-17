// $Id: JFactory.h 1733 2006-05-02 17:17:32Z davidl $


#ifndef _JFACTORY_H_
#define _JFACTORY_H_



#include "JObject.h"

#include <iostream>
#include <vector>
#include <string>
using std::vector;
using std::string;

#include "JEventLoop.h"
#include "JFactory_base.h"
#include "JEvent.h"

// The following is here just so we can use ROOT's THtml class to generate documentation.
#if defined(__CINT__) || defined(__CLING__)
class pthread_mutex_t;
typedef unsigned long pthread_t;
typedef unsigned long oid_t;
#endif

// Place everything in JANA namespace
namespace jana{

///
/// JANA Data Factory (base class for algorithms)
///
/// All data (except that read from the data source) must
/// be derived from the data that was read from the source.
/// One JFactory object should exist for each type of data
/// that is "generated". For example: Clusters in a calorimeter
/// are generated from individual "Hits" in the calorimeter.
/// 
/// JFactory is the templated base class which all factories
/// are derived from.
///
/// At the user level, factory classes will be defined
/// which inherit from this templated base class. By
/// inheriting from a template, the derived factory class
/// will automatically have a high degree of type safety
/// since the "_data" vector will be specific to the type
/// of objects the factory produces. This class (JFactory)
/// inherits from JFactory_base so that all factories can
/// be treated equally (polymorphism) by the JEventLoop object.
///
/// Instantiating a JFactory<T> object itself would be pointless
/// since they would use the default  init(), brun(),evnt(),erun(),
/// and fini()
/// methods from JEventProcessor which do nothing. Instead,
/// A new class should be derived from this one which implements
/// its own init(), brun(),evnt(),erun(), and fini() methods.

//-----------------------
// class JFactory
//-----------------------
template<class T>
class JFactory:public JFactory_base{

	friend class JEventLoop;
	
	public:
		JFactory(const char *tag="");
		~JFactory();

		enum data_origin_t{
			ORIGIN_NONE,
			ORIGIN_FACTORY,
			ORIGIN_EXTERNAL
		};
		
		vector<void*>& Get(void);
		jerror_t Get(vector<const T*> &d);
		int GetNrows(bool force_call_to_get=false, bool do_not_call_get=false);
		data_origin_t GetDataOrigin(void){return data_origin;}
		inline const char* className(void){return T::static_className();}
		inline const char* GetDataClassName(void){return className();}
		inline void toStrings(vector<vector<pair<string,string> > > &items, bool append_types=false) const;
		virtual const char* Tag(void){return tag_str;}
		inline int GetDataClassSize(void){return sizeof(T);}
		inline int GetEventCalled(void){return evnt_called;}
		inline int GetCheckSourceFirst(void){return !use_factory;}
		jerror_t CopyFrom(vector<const T*> &data);
		jerror_t CopyTo(vector<T*> &data);
		jerror_t CopyTo(vector<JObject*> &data);
		const T* GetByIDT(JObject::oid_t id);
		const JObject* GetByID(JObject::oid_t id){return dynamic_cast<const JObject*>(GetByIDT(id));}
		

	protected:
		vector<T*> _data;
		vector<void*>_vdata;
		int use_factory;
		const char* tag_str;
		
		jerror_t Reset(void);
		jerror_t HardReset(void);
		void SetFactoryPointers(void);
		
		data_origin_t data_origin;
};


// The following is here just so we can use ROOT's THtml class to generate documentation.
#if !defined(__CINT__) && !defined(__CLING__)

//-------------
// JFactory
//-------------
template<class T>
JFactory<T>::JFactory(const char *tag)
{
	/// This is a base class that specific factories inherit from.
	/// my_devent will be kept and used to allow this factory to
	/// access other factories.

	// make sure vector is empty
	_data.clear(); // probably unnecessary
	
	// clear flags
	flags = WRITE_TO_OUTPUT;
	use_factory = 0;
	busy = 0;
	tag_str = tag;
	Ncalls_to_Get = 0;
	Ncalls_to_evnt = 0;

	// Allow any factory to have its debug_level set via environment variable
	debug_level = 0;
	string envar = string() + "DEBUG_" + GetDataClassName();
	char *ptr = getenv(envar.c_str());
	if(ptr)debug_level = atoi(ptr);
}

//-------------
// ~JFactory
//-------------
template<class T>
JFactory<T>::~JFactory()
{
	/// Delete all objects in _data container.
	HardReset();
}

//-------------
// Get
//-------------
template<class T>
vector<void*>& JFactory<T>::Get(void)
{
	/// Return a STL vector of pointers to the objects produced by the
	/// factory. The pointers are cast as void* so this can be
	/// accessed through the JFactory_base base class. This just
	/// dispatches to the type specific Get(vector<const T*> &)
	/// method.
	///
	/// IMPORTANT NOTE ON MULTIPLE INHERITANCE:
	/// This will use a static_cast to return void* pointers to the
	/// start of the JObject data. If the data object class inherits
	/// from multiple classes and JObject is not the first one, then 
	/// the void* pointers will be pointing to the middle of the object
	/// data. To obtain the pointer to the actual start of the object
	/// one needs to first cast to JObject* and then static cast to
	/// the correct type.
	vector<const T*> d;
	Get(d);
	
	// Copy the pointers into a vector of void*s.
	_vdata.clear();
	for(unsigned int i=0;i<_data.size();i++){
		// Here we use a static_cast to make sure a pointer to
		// the JObject data is stored. In the case of multiple
		// inheritance, data from another class may precede the
		// JObject data so the void* pointers may not actually
		// point to the start of the object. We do this so other
		// JANA facilities can make use of the JObject part of
		// the class without having to know how to cast it to the
		// full object type.
		void *vpointer = static_cast<JObject*>(_data[i]);
		_vdata.push_back(vpointer);
	}
	
	return _vdata;
}

//-------------
// Get
//-------------
template<class T>
jerror_t JFactory<T>::Get(vector<const T*> &d)
{
	/// Copy pointers to the objects produced by the factory into
	/// the vector reference passed. 
	/// Note that this method is accessed primarily from two places:
	/// This main one is JEventLoop::GetFromFactory which is called
	/// from JEventLoop::Get , the primary access method for factories
	/// and event processors.
	/// The second is through the JFactory<T>::Get() which is called
	/// through the JFactory_base::Get virtual method.
	/// That is used mainly by things that need to loop over all
	/// objects of all factories.
	///
	/// This method will check first to make sure this factory hasn't
	/// already been called for this event.
	///
	/// This also uses a busy flag to ensure we're not called
	/// recursively. i.e. we call a factory who calls another
	/// factory who eventually calls us. An exception is thrown
	/// (type jerror_t) with a value INFINITE_RECURSION if that
	/// situation is detected.
	
	
	// If evnt_called is set, then just copy the pointers and return
	if(evnt_called)return CopyFrom(d);
	
	// Check for infinite recursion through factory dependancies
	if(busy){
		string mess = string("Infinite recursion detected in JFactory<T>::Get for ")+GetDataClassName()+":"+Tag();
		std::cerr<<mess<<std::endl;
		throw JException(mess);
	}
	busy++;
	if(busy!=1){
		string mess = string("Infinite recursion detected in JFactory<T>::Get for ")+GetDataClassName()+":"+Tag();
		std::cerr<<mess<<std::endl;
		throw JException(mess);
	}	
	// Grab the current event and run numbers
	uint64_t event_number = eventLoop->GetJEvent().GetEventNumber();
	int32_t run_number = eventLoop->GetJEvent().GetRunNumber();
	
	// Make sure we're initialized
	if(!init_called){
		try{
			init();
		}catch(std::exception &e){
			string tag_plus = string(Tag()) + " (init)";
			JEventLoop::error_call_stack_t cs = {GetDataClassName(), tag_plus.c_str(), __FILE__, __LINE__};
			eventLoop->AddToErrorCallStack(cs);
			busy = 0; // clear busy flag since where exiting early and no longer "busy"
			throw e;
		}
		init_called = 1;
	}
	
	// Call erun if appropriate
	if(run_number!=brun_runnumber){
		if(brun_called && !erun_called){
			try{
				erun();
			}catch(std::exception &e){
				string tag_plus = string(Tag()) + " (erun)";
				JEventLoop::error_call_stack_t cs = {GetDataClassName(), tag_plus.c_str(), __FILE__, __LINE__};
				eventLoop->AddToErrorCallStack(cs);
				busy = 0; // clear busy flag since where exiting early and no longer "busy"
				throw e;
			}
			erun_called = 1;
		}
		brun_called = 0;
	}
	
	// Check if event boundary has been crossed
	if(brun_called && eventLoop->CheckEventBoundary(event_number, brun_eventnumber)){
		// This event and the event for the last time we were called
		// span an event boundary. Force brun to be called again.
		brun_called = 0;
	}
	
	// Call brun if it hasn't been yet
	if(!brun_called){
		try{
			brun(eventLoop, run_number);
		}catch(std::exception &e){
			string tag_plus = string(Tag()) + " (brun)";
			JEventLoop::error_call_stack_t cs = {GetDataClassName(), tag_plus, __FILE__, __LINE__};
			eventLoop->AddToErrorCallStack(cs);
			busy = 0; // clear busy flag since where exiting early and no longer "busy"
			throw e;
		}
		brun_called = 1;
		erun_called = 0;
		brun_runnumber = run_number;
		brun_eventnumber = event_number;
	}
	
	// Call evnt routine to generate data
	try{
		Ncalls_to_evnt++;
		evnt(eventLoop, event_number);
		CopyFrom(d);
	}catch(std::exception &e){
		string tag_plus = string(Tag()) + " (evnt)";
		JEventLoop::error_call_stack_t cs = {GetDataClassName(), tag_plus.c_str(), __FILE__, __LINE__};
		eventLoop->AddToErrorCallStack(cs);
		busy = 0; // clear busy flag since where exiting early and no longer "busy"
		throw e;
	}
	evnt_called = 1;
	busy=0;
	
	return NOERROR;
}

//-------------
// GetNrows
//-------------
template<class T>
int JFactory<T>::GetNrows(bool force_call_to_get, bool do_not_call_get)
{
	/// Return the number of objects for this factory for the
	/// current event. If the objects have not yet been created
	/// for the event, this will cause them to be generated 
	/// (or retreived from the source) UNLESS the "do_not_call_get"
	/// flag is set to true. If "do_not_call_get" is true then
	/// "force_call_to_get" is ignored and the number of objects
	/// already existing is returned. This is useful for things
	/// like janadump where it wants to control which factories
	/// get called.
	///
	/// If the optional "force_call_to_get" parameter is set to true,
	/// Then the "Get" method will get called regardess of whether it
	/// has already been called for this event. This is only useful for
	/// when AUTOACTIVATE'd factories are used AND the call stack is
	/// being recorded for use by something like the janadot plugin.
	/// Forcing the call to "Get" will then put an entry on the call stack
	/// leading to a line being drawn from the AutoActivated polygon to
	/// all factories listed as being AUTOACTIVATE.
	if( !do_not_call_get ){
		if(!evnt_called || force_call_to_get){
			// In order to get the objects from the source, the Get() method
			// of the JEventLoop object must be called. This may seem
			// convoluted, but it's neccessary for things like janadump
			// that only have a list of JFactory_base pointers and
			// are not able to call the templated JEventLoop::Get()
			// method directly.
			// Note that we must use the Tag() method so it will pass
			// the tag from the subclass (if any). The tag_str member
			// is only in JFactory for when sources must auto-instantiate
			// a JFactory<> class.
			vector<const T*> d; // dummy placeholder
			eventLoop->Get(d,Tag());
		}
	}
	
	return _data.size();
}

//-------------
// Reset
//-------------
template<class T>
jerror_t JFactory<T>::Reset(void)
{
	/// Clear out the factories current contents unless the
	/// PERSISTANT flag is set.
	if(flags & (unsigned int)PERSISTANT)return NOERROR;

	// don't reset the evnt_called flag for persistent data because this
	// will force evnt to be called next event therby regenerating
	// the data
	
	return HardReset();
}

//-------------
// HardReset
//-------------
template<class T>
jerror_t JFactory<T>::HardReset(void)
{
	
	/// Clear out the factories current contents.
	if(!TestFactoryFlag(NOT_OBJECT_OWNER)){
		for(unsigned int i=0;i<_data.size();i++){
			delete _data[i];
		}
	}
	_data.clear();

	evnt_called = 0;
	
	return NOERROR;
}

//-------------
// SetFactoryPointers
//-------------
template<class T>
void JFactory<T>::SetFactoryPointers(void)
{
	/// Set the JFactory_base pointers for all JObjects in _data to
	/// point back to this factory. Each item in _data is dynamic_cast
	/// as a JObject* first and if that succeeds, then the object's
	/// SetFactoryPointer() method is called.
	///
	/// This is called from JEventLoop::GetFromFactory so that the
	/// pointers are set just after object creation so that the user
	/// doesn't have to explicitly.
	for(unsigned int i=0; i<_data.size(); i++){
		JObject *obj = dynamic_cast<JObject*>(_data[i]);
		if(obj)obj->SetFactoryPointer(this);
	}
}

//-------------
// CopyTo
//-------------
template<class T>
jerror_t JFactory<T>::CopyTo(vector<T*> &data)
{
	/// Copy object pointers into factory. Object ownership is
	/// transferred to the factory here so the caller should not
	/// delete the objects after calling this unless the factory's
	/// NOT_OBJECT OWNER flag is set (see
	/// SetFactoryFlag(JFactory_base::NOT_OBJECT_OWNER))
	/// This is how an external source such as a file can create
	/// the objects and put them in the factory. The objects will
	/// then be treated as though they were generated by the factory.
	/// i.e. they will be returned in subsequent calls to Get()
	/// and deleted at the start of the next event (sans 
	/// the NOT_OBJECT_OWNER or PERSISTANT flags).

	// Set flag so subsequent calls for this event will return this
	// data.
	evnt_called = 1;
		
	// Just copy into the _vdata vector since _data is not used outside
	// of the factory.
	_data.clear();
	for(unsigned int i=0;i<data.size();i++){
		_data.push_back(data[i]);
	}

	return NOERROR;
}

//-------------
// CopyTo
//-------------
template<class T>
jerror_t JFactory<T>::CopyTo(vector<JObject*> &data)
{
	/// Copy object pointers into factory. Object ownership is
	/// transferred to the factory here so the caller should not
	/// delete the objects after calling this unless the factory's
	/// NOT_OBJECT OWNER flag is set (see
	/// SetFactoryFlag(JFactory_base::NOT_OBJECT_OWNER))
	/// This is used if the caller doesn't know precisely the type
	/// of data objects this factory holds. This primarily happens
	/// if they have a pointer to the JFactory_base class and use
	/// the virtual CopyTo method from there which maps to this one.
	///
	/// IMPORTANT: Because this only copies objects that are successfully
	/// dynamic_cast into the factory's type, not all objects may be
	/// placed in this factory. This is handy if one has a single
	/// vector of several types that then don't have to be pre-sorted.
	/// On the other hand, this could potentially lead to memory
	/// leaks if the objects not copied into this factory are not
	/// deleted elsewhere.
	///
	/// If you have pointers to the actual object types (i.e. not
	/// just the JObject pointers) you should use those since it
	/// avoids the expensive dynamic_cast operation on each pointer.
	///
	/// The objects copied into this factory will
	/// be treated as though they were generated by the factory.
	/// i.e. they will be returned in subsequent calls to Get()
	/// and deleted at the start of the next event (sans 
	/// the NOT_OBJECT_OWNER or PERSISTANT flags).

	// Set flag so subsequent calls for this event will return this
	// data.
	evnt_called = 1;
		
	// Just copy into the _vdata vector since _data is not used outside
	// of the factory.
	_data.clear();
	for(unsigned int i=0;i<data.size();i++){
		T* ptr = dynamic_cast<T*>(data[i]);
		if(ptr != NULL) _data.push_back(ptr);
	}

	return NOERROR;
}

//-------------
// CopyFrom
//-------------
template<class T>
jerror_t JFactory<T>::CopyFrom(vector<const T*> &data)
{
	/// Copy object pointers from private vector into "data"
	/// vector. This does not check if the evnt() routine
	/// was called, and it does not generate the data objects
	/// (use the Get(vector<const T*>) method for that).
	/// This only copies pointers to already existing objects.
	for(unsigned int i=0;i<_data.size(); i++)data.push_back(_data[i]);
	Ncalls_to_Get++;
	
	return NOERROR;
}

//-------------
// GetByIDT
//-------------
template<class T>
const T* JFactory<T>::GetByIDT(JObject::oid_t id)
{
	for(unsigned int i=0;i<_data.size();i++)
		if(_data[i]->id == id)return (const T*)_data[i];
	return NULL;
}

//-------------
// toStrings
//-------------
template<class T>
void JFactory<T>::toStrings(vector<vector<pair<string,string> > > &items, bool append_types) const
{
	/// Get the data for all objects already created by this factory for the
	/// given event by calling the toStrings() method of each one.
	///
	/// Note that this will not activate the factory to generate the objects
	/// if they do not already exist.
	///
	/// The value of <i>append_types</i> is used to set the append_types
	/// flag for each of the objects prior to calling its <i>toStrings</i>
	/// method. Afterwards, the value of the objects' append_types flag
	/// is reset to whatever value it had upon entry. Please see the documentation
	/// of the AddString method of JObject for more details on what this
	/// flag does.
	///
	/// The items vector is cleared upon entry.
	
	items.clear();
	
	for(unsigned int i=0;i<_data.size();i++){
		vector<pair<string, string> > myitems;
		bool save_append_types = _data[i]->GetAppendTypes();
		_data[i]->SetAppendTypes(append_types);
		_data[i]->toStrings(myitems);
		_data[i]->SetAppendTypes(save_append_types);
		if(myitems.size()>0)items.push_back(myitems);
	}
}

#endif //__CINT__  __CLING__


} // Close JANA namespace



#endif // _JFACTORY_H_
