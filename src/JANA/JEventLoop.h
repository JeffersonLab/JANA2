// $Id: JEventLoop.h 1763 2006-05-10 14:29:25Z davidl $
//
//    File: JEventLoop.h
// Created: Wed Jun  8 12:30:51 EDT 2005
// Creator: davidl (on Darwin wire129.jlab.org 7.8.0 powerpc)
//

#ifndef _JEventLoop_
#define _JEventLoop_

#include <vector>
#include <string>
#include <utility>
using std::vector;
using std::string;

#include "jerror.h"
#include "JObject.h"
#include "JException.h"
#include "JEvent.h"
#include "JFactory_base.h"
#include "JCalibration.h"
#include "JGeometry.h"

// The following is here just so we can use ROOT's THtml class to generate
// documentation.
#ifdef __CINT__
class pthread_mutex_t;
typedef unsigned long pthread_t;
#endif


// Place everything in JANA namespace
namespace jana{


template<class T> class JFactory;
class JApplication;
class JEventProcessor;

class JEventLoop{
	public:
		JEventLoop(JApplication *app); ///< Constructor
		virtual ~JEventLoop(); ////< Destructor
		virtual const char* className(void){return static_className();}
		static const char* static_className(void){return "JEventLoop";}

		enum data_source_t{
			DATA_NOT_AVAILABLE = 1,
			DATA_FROM_CACHE,
			DATA_FROM_SOURCE,
			DATA_FROM_FACTORY
		};
		
		typedef struct{
			string caller_name;
			string caller_tag;
			string callee_name;
			string callee_tag;
			clock_t start_time;
			clock_t end_time;
			data_source_t data_source;
		}call_stack_t;

		typedef struct{
			const char* factory_name;
			const char* tag;
			const char* filename;
			int line;
		}error_call_stack_t;
		
		JApplication* GetJApplication(void){return app;} ///< Get pointer to the JApplication object
		void RefreshProcessorListFromJApplication(void); ///< Re-copy the list of JEventProcessors from JApplication
		virtual jerror_t AddFactory(JFactory_base* factory); ///< Add a factory
		jerror_t RemoveFactory(JFactory_base* factory); ///< Remove a factory
		JFactory_base* GetFactory(const string data_name, const char *tag=""); ///< Get a specific factory pointer
		vector<JFactory_base*> GetFactories(void){return factories;} ///< Get all factory pointers
		void GetFactoryNames(vector<string> &factorynames); ///< Get names of all factories in name:tag format
		void GetFactoryNames(map<string,string> &factorynames); ///< Get names of all factories in map with key=name, value=tag
		map<string,string> GetDefaultTags(void){return default_tags;}
		jerror_t ClearFactories(void); ///< Reset all factories in preparation for next event.
		jerror_t PrintFactories(int sparsify=0); ///< Print a list of all factories.
		jerror_t Print(const string data_name, const char *tag=""); ///< Print the data of the given type

		JCalibration* GetJCalibration();
		template<class T> bool GetCalib(string namepath, map<string,T> &vals);
		template<class T> bool GetCalib(string namepath, vector<T> &vals);

		JGeometry* GetJGeometry();
		template<class T> bool GetGeom(string namepath, map<string,T> &vals);
		template<class T> bool GetGeom(string namepath, T &val);

		jerror_t Loop(void); ///< Loop over events
		jerror_t OneEvent(void); ///< Process a single event
		inline void Pause(void){pause = 1;} ///< Pause event processing
		inline void Resume(void){pause = 0;} ///< Resume event processing
		inline void Quit(void){quit = 1;} ///< Clean up and exit the event loop
		inline bool GetQuit(void){return quit;}
		void QuitProgram(void);
		
		template<class T> JFactory<T>* Get(vector<const T*> &t, const char *tag=""); ///< Get data object pointers from (source or factory)
		template<class T> JFactory<T>* GetFromFactory(vector<const T*> &t, const char *tag="", data_source_t &data_source=NULL); ///< Get data object pointers from factory
		template<class T> jerror_t GetFromSource(vector<const T*> &t, JFactory_base *factory=NULL); ///< Get data object pointers from source.
		inline JEvent& GetJEvent(void){return event;} ///< Get pointer to the current JEvent object.
		inline void SetJEvent(JEvent *event){this->event = *event;} ///< Set the JEvent pointer.
		inline void SetAutoFree(int auto_free){this->auto_free = auto_free;} ///< Set the Auto-Free flag on/off
		inline pthread_t GetPThreadID(void){return pthread_id;} ///< Get the pthread of the thread to which this JEventLoop belongs
		

		inline vector<call_stack_t> GetCallStack(void){return call_stack;} ///< Get the current factory call stack
		inline void AddToErrorCallStack(error_call_stack_t &cs){error_call_stack.push_back(cs);} ///< Add layer to the factory call stack
		inline vector<error_call_stack_t> GetErrorCallStack(void){return error_call_stack;} ///< Get the current factory error call stack
		void PrintErrorCallStack(void); ///< Print the current factory call stack
		
		const JObject* FindByID(JObject::oid_t id); ///< Find a data object by its identifier.
		template<class T> const T* FindByID(JObject::oid_t id); ///< Find a data object by its type and identifier
		JFactory_base* FindOwner(const JObject *t); ///< Find the factory that owns a data object by pointer
		JFactory_base* FindOwner(JObject::oid_t id); ///< Find a factory that owns a data object by identifier

	private:
		JEvent event;
		vector<JFactory_base*> factories;
		vector<JEventProcessor*> processors;
		vector<error_call_stack_t> error_call_stack;
		vector<call_stack_t> call_stack;
		JApplication *app;
		double *heartbeat;
		int pause;
		int quit;
		int auto_free;
		pthread_t pthread_id;
		map<string, string> default_tags;
		vector<pair<string,string> > auto_activated_factories;
		bool record_call_stack;
		string caller_name;
		string caller_tag;
};


//-------------
// Get
//-------------
template<class T> 
JFactory<T>* JEventLoop::Get(vector<const T*> &t, const char *tag)
{
	/// Retrieve or generate the array of objects of
	/// type T for the curent event being processed
	/// by this thread.
	///
	/// By default, preference is given to reading the
	/// objects from the data source(e.g. file) before generating
	/// them in the factory. A flag exists in the factory
	/// however to change this so that the factory is
	/// given preference.
	///
	/// Note that regardless of the setting of this flag,
	/// the data are only either read in or generated once.
	/// Ownership of the objects will always be with the
	/// factory so subsequent calls will always return pointers to
	/// the same data.
	///
	/// If the factory is called on to generate the data,
	/// it is done by calling the factory's Get() method
	/// which, in turn, calls the evnt() method. 
	/// 
	/// First, we just call the GetFromFactory() method.
	/// It will make the initial decision as to whether
	/// it should look in the source first or not. If
	/// it returns NULL, then the factory couldn't be
	/// found so we automatically try the file.
	///
	/// Note that if no factory exists to hold the objects
	/// from the file, one can be created automatically
	/// providing the <i>JANA:AUTOFACTORYCREATE</i>
	/// configuration parameter is set.
	
	// If we are trying to keep track of the call stack then we
	// need to add a new call_stack_t object to the the list
	// and initialize it with the start time and caller/callee
	// info.
	call_stack_t cs;
	if(record_call_stack){
		cs.caller_name = caller_name;
		cs.caller_tag = caller_tag;
		cs.callee_name = T::static_className();
		cs.callee_tag = tag;
		caller_name = cs.callee_name;
		caller_tag = cs.callee_tag;
		cs.start_time = clock();
	}

	// Get the data (or at least try to)
	JFactory<T>* factory=NULL;
	try{
		factory = GetFromFactory(t, tag, cs.data_source);
		if(!factory){
			// No factory exists for this type and tag. It's possible
			// that the source may be able to provide the objects
			// but it will need a place to put them. We can create a
			// dumb JFactory just to hold the data in case the source
			// can provide the objects. Before we do though, make sure
			// the user condones this via the presence of the
			// "JANA:AUTOFACTORYCREATE" config parameter.
			string p;
			gPARMS->GetParameter("JANA:AUTOFACTORYCREATE", p);
			if(p.size()==0){
				std::cout<<std::endl;
				_DBG__;
				std::cout<<"No factory of type \""<<T::static_className()<<"\" with tag \""<<tag<<"\" exists."<<std::endl;
				std::cout<<"If you are reading objects from a file, I can auto-create a factory"<<std::endl;
				std::cout<<"of the appropriate type to hold the objects, but this feature is turned"<<std::endl;
				std::cout<<"off by default. To turn it on, set the \"JANA:AUTOFACTORYCREATE\""<<std::endl;
				std::cout<<"configuration parameter. This can usually be done by passing the"<<std::endl;
				std::cout<<"following argument to the program from the command line:"<<std::endl;
				std::cout<<std::endl;
				std::cout<<"   -PJANA:AUTOFACTORYCREATE=1"<<std::endl;
				std::cout<<std::endl;
				std::cout<<"Note that since the most commonly expected occurance of this situation."<<std::endl;
				std::cout<<"is an error, the program will quit now."<<std::endl;
				std::cout<<std::endl;
				QuitProgram();
			}else{
				AddFactory(new JFactory<T>(tag));
				std::cout<<__FILE__<<":"<<__LINE__<<" Auto-created "<<T::static_className()<<":"<<tag<<" factory"<<std::endl;
			
				// Now try once more. The GetFromFactory method will call
				// GetFromSource since it's empty.
				factory = GetFromFactory(t, tag, cs.data_source);
			}
		}
	}catch(JException *exception){
		// Uh-oh, an exception was thrown. Add us to the call stack
		// and re-throw the exception
		error_call_stack_t ecs;
		ecs.factory_name = T::static_className();
		ecs.tag = tag;
		if(exception!=NULL && error_call_stack.size()==0){
			//ecs.filename = exception->filename;
			//ecs.line = exception->line;
		}else{
			ecs.filename = NULL;
		}
		error_call_stack.push_back(ecs);
		throw(exception);
	}
	
	// If recording the call stack, update the end_time field
	if(record_call_stack){
		cs.end_time = clock();
		caller_name = cs.caller_name;
		caller_tag = cs.caller_tag;
		call_stack.push_back(cs);
	}
	
	return factory;
}

//-------------
// GetFromFactory
//-------------
template<class T> 
JFactory<T>* JEventLoop::GetFromFactory(vector<const T*> &t, const char *tag, data_source_t &data_source)
{
	// We need to find the factory providing data type T with
	// tag given by "tag". 
	vector<JFactory_base*>::iterator iter=factories.begin();
	JFactory<T> *factory = NULL;
	string className(T::static_className());
	
	// Check if a tag was specified for this data type to use for the
	// default.
	const char *mytag = tag==NULL ? "":tag; // prtection against NULL tags
	if(strlen(mytag)==0){
		map<string, string>::const_iterator iter = default_tags.find(className);
		if(iter!=default_tags.end())tag = iter->second.c_str();
	}
	
	for(; iter!=factories.end(); iter++){
		// It turns out a long standing bug in g++ makes dynamic_cast return
		// zero improperly when used on objects created on one side of
		// a dynamically shared object (DSO) and the cast occurs on the 
		// other side. I saw bug reports ranging from 2001 to 2004. I saw
		// saw it first-hand on LinuxEL4 using g++ 3.4.5. This is too bad
		// since it is much more elegant (and safe) to use dynamic_cast.
		// To avoid this problem which can occur with plugins, we check
		// the name of the data classes are the same. (sigh)
		//factory = dynamic_cast<JFactory<T> *>(*iter);
		if(className == (*iter)->GetDataClassName())factory = (JFactory<T>*)(*iter);
		if(factory == NULL)continue;
		const char *factag = factory->Tag()==NULL ? "":factory->Tag();
		if(!strcmp(factag, tag)){
			break;
		}else{
			factory=NULL;
		}
	}
	
	// If factory not found, just return now
	if(!factory){
		data_source = DATA_NOT_AVAILABLE;
		return NULL;
	}
	
	// OK, we found the factory. If the evnt() routine has already
	// been called, then just call the factory's Get() routine
	// to return a copy of the existing data
	if(factory->evnt_was_called()){
		factory->CopyFrom(t);
		data_source = DATA_FROM_CACHE;
		return factory;
	}
	
	// Next option is to get the objects from the data source
	if(factory->GetCheckSourceFirst()){
		// If the object type/tag is found in the source, it
		// will return NOERROR, even if there are zero instances
		// of it. If it is not available in the source then it
		// will return OBJECT_NOT_AVAILABLE.
		
		jerror_t err = GetFromSource(t, factory);
		if(err == NOERROR){
			// A return value of NOERROR means the source had the objects
			// even if there were zero of them.(If the source had no
			// information about the objects OBJECT_NOT_AVAILABLE would 
			// have been returned.)
			// The GetFromSource() call will eventually lead to a call to
			// the GetObjects() method of the concrete class derived
			// from JEventSource. That routine should copy the object
			// pointers into the factory using the factory's CopyTo()
			// method which also sets the evnt_called flag for the factory.
			// Note also that the "t" vector is then filled with a call
			// to the factory's CopyFrom() method in JEvent::GetObjects().
			// All we need to do now is just return the factory pointer.

			data_source = DATA_FROM_SOURCE;
			return factory;
		}
	}
		
	// OK. It looks like we have to have the factory make this.
	// Get pointers to data from the factory.
	factory->Get(t);
	data_source = DATA_FROM_FACTORY;
	
	return factory;
}

//-------------
// GetFromSource
//-------------
template<class T> 
jerror_t JEventLoop::GetFromSource(vector<const T*> &t, JFactory_base *factory)
{
	/// This tries to get objects from the event source.
	/// "factory" must be a valid pointer to a JFactory
	/// object since that will take ownership of the objects
	/// created by the source.
	/// This should usually be called from JEventLoop::GetFromFactory
	/// which is called from JEventLoop::Get. The latter will
	/// create a dummy JFactory of the proper flavor and tag if
	/// one does not already exist so if objects exist in the
	/// file without a corresponding factory to create them, they
	/// can still be used.
	if(!factory)throw OBJECT_NOT_AVAILABLE;
	
	return event.GetObjects(t, factory);
}

//-------------
// FindByID
//-------------
template<class T> 
const T* JEventLoop::FindByID(JObject::oid_t id)
{
	/// This is a templated method that can be used in place
	/// of the non-templated FindByID(oid_t) method if one knows
	/// the class of the object with the specified id.
	/// This method is faster than calling the non-templated
	/// FindByID and dynamic_cast-ing the JObject since
	/// this will only search the objects of factories that
	/// produce the desired data type.
	/// This method will cast the JObject pointer to one
	/// of the specified type. To use this method,
	/// a type is specified in the call as follows:
	///
	/// const DMyType *t = loop->FindByID<DMyType>(id);
	
	// Loop over factories looking for ones that provide
	// specified data type.
	for(uint i=0; i<factories.size(); i++){
		if(factories[i]->GetDataClassName() != T::className())continue;

		// This factory provides data of type T. Search it for
		// the object with the specified id.
		const JObject *my_obj = factories[i]->GetByID(id);
		if(my_obj)return dynamic_cast<const T*>(my_obj);
	}

	return NULL;
}

//-------------
// GetCalib (map)
//-------------
template<class T>
bool JEventLoop::GetCalib(string namepath, map<string,T> &vals)
{
	/// Get the JCalibration object from JApplication for the run number of
	/// the current event and call its Get() method to get the constants.

	// Note that we could do this by making "vals" a generic type T thus, combining
	// this with the vector version below. However, doing this explicitly will make
	// it easier for the user to understand how to call us.

	vals.clear();

	JCalibration *calib = GetJCalibration();
	if(!calib){
		_DBG_<<"Unable to get JCalibration object for run "<<event.GetRunNumber()<<std::endl;
		return true;
	}
	
	return calib->Get(namepath, vals);
}

//-------------
// GetCalib (vector)
//-------------
template<class T> bool JEventLoop::GetCalib(string namepath, vector<T> &vals)
{
	/// Get the JCalibration object from JApplication for the run number of
	/// the current event and call its Get() method to get the constants.

	vals.clear();

	JCalibration *calib = GetJCalibration();
	if(!calib){
		_DBG_<<"Unable to get JCalibration object for run "<<event.GetRunNumber()<<std::endl;
		return true;
	}
	
	return calib->Get(namepath, vals);
}


//-------------
// GetGeom (map)
//-------------
template<class T>
bool JEventLoop::GetGeom(string namepath, map<string,T> &vals)
{
	/// Get the JGeometry object from JApplication for the run number of
	/// the current event and call its Get() method to get the constants.

	// Note that we could do this by making "vals" a generic type T thus, combining
	// this with the vector version below. However, doing this explicitly will make
	// it easier for the user to understand how to call us.

	vals.clear();

	JGeometry *geom = GetJGeometry();
	if(!geom){
		_DBG_<<"Unable to get JGeometry object for run "<<event.GetRunNumber()<<std::endl;
		return true;
	}
	
	return geom->Get(namepath, vals);
}

//-------------
// GetGeom (atomic)
//-------------
template<class T> bool JEventLoop::GetGeom(string namepath, T &val)
{
	/// Get the JCalibration object from JApplication for the run number of
	/// the current event and call its Get() method to get the constants.

	JGeometry *geom = GetJGeometry();
	if(!geom){
		_DBG_<<"Unable to get JGeometry object for run "<<event.GetRunNumber()<<std::endl;
		return true;
	}
	
	return geom->Get(namepath, val);
}

} // Close JANA namespace



#endif // _JEventLoop_

