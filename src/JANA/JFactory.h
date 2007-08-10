// $Id: JFactory.h 1733 2006-05-02 17:17:32Z davidl $


#ifndef _JFACTORY_H_
#define _JFACTORY_H_

#define HDCLASSDEF(T) \
	static const char* className(){return #T;}


#include "JObject.h"

#ifndef __CINT__  // disable this file from ROOT interpreter

#include <vector>
#include <string>
using namespace std;

#include "JEventLoop.h"
#include "JFactory_base.h"
#include "JEvent.h"

///
/// Hall-D Data Factory
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
/// and fini().
/// methods from JEventProcessor which do nothing. Instead,
/// A new class should be derived from this one which implements
/// its own init(), brun(),evnt(),erun(), and fini() methods.

//-----------------------
// class JFactory
//-----------------------
template<class T>
class JFactory:public JFactory_base{
	
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
		int GetNrows(void);
		data_origin_t GetDataOrigin(void){return data_origin;}
		inline const char* className(void){return T::className();}
		inline const char* dataClassName(void){return className();}
		virtual const char* Tag(void){return tag_str;}
		inline int dataClassSize(void){return sizeof(T);}
		inline int GetEventCalled(void){return evnt_called;}
		inline int CheckSourceFirst(void){return !use_factory;}
		jerror_t CopyFrom(vector<const T*> &data);
		jerror_t CopyTo(vector<T*> &data);
		const T* GetByIDT(oid_t id);
		const JObject* GetByID(oid_t id){return dynamic_cast<const JObject*>(GetByIDT(id));}
		

	protected:
		vector<T*> _data;
		vector<void*>_vdata;
		int use_factory;
		const char* tag_str;
		
		jerror_t Reset(void);
		jerror_t HardReset(void);
		
		data_origin_t data_origin;
};



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
	string envar = string() + "DEBUG_" + dataClassName();
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
	/// factory. The pointers are type cast as void* so this can be
	/// accessed through the JFactory_base base class. This just
	/// dispatches to the type specific Get(vector<const T*> &)
	/// method.
	vector<const T*> d;
	Get(d);
	
	// Copy the pointers into a vector of void*s.
	_vdata.clear();
	for(unsigned int i=0;i<_data.size();i++)_vdata.push_back((void*)_data[i]);
	
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
	
	Ncalls_to_Get++;
	
	// If evnt_called is set, then just copy the pointers and return
	if(evnt_called)return CopyFrom(d);
	
	// Check for infinite recursion through factory dependancies
	if(busy)throw(INFINITE_RECURSION);
	busy++;
	if(busy!=1)throw(INFINITE_RECURSION);  // Should we use a mutex here?
	
	// Grab the current event and run numbers
	int event_number = eventLoop->GetJEvent().GetEventNumber();
	int run_number = eventLoop->GetJEvent().GetRunNumber();
	
	// Make sure we're initialized
	if(!init_called){
		init();
		init_called = 1;
	}
	
	// Call brun routine if run number has changed or it's not been called
	if(run_number!=brun_runnumber){
		if(brun_called && !erun_called){
			erun();
			erun_called = 1;
		}
		brun_called = 0;
	}
	if(!brun_called){
		brun(eventLoop, run_number);
		brun_called = 1;
		erun_called = 0;
		brun_runnumber = run_number;
	}
	
	// Call evnt routine to generate data
	try{
		Ncalls_to_evnt++;
		evnt(eventLoop, event_number);
		CopyFrom(d);
	}catch(JException *exception){
		JEventLoop::call_stack_t cs;
		cs.factory_name = dataClassName();
		cs.tag = Tag();
		cs.filename = __FILE__;
		cs.line = __LINE__;
		eventLoop->AddToCallStack(cs);
		throw exception;
	}
	evnt_called = 1;
	busy=0;
	
	return NOERROR;
}

//-------------
// GetNrows
//-------------
template<class T>
int JFactory<T>::GetNrows(void)
{
	/// Return the number of objects for this factory for the
	/// current event. If the objects have not yet been created
	/// for the event, this will cause them to be generated 
	/// (or retreived from the source).
	if(!evnt_called){
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
	
	return NOERROR;
}

//-------------
// GetByIDT
//-------------
template<class T>
const T* JFactory<T>::GetByIDT(oid_t id)
{
	for(unsigned int i=0;i<_data.size();i++)
		if(_data[i]->id == id)return (const T*)_data[i];
	return NULL;
}

#endif // __CINT__

#endif // _JFACTORY_H_
