// $Id: JEventLoop.h 1763 2006-05-10 14:29:25Z davidl $
//
//    File: JEventLoop.h
// Created: Wed Jun  8 12:30:51 EDT 2005
// Creator: davidl (on Darwin wire129.jlab.org 7.8.0 powerpc)
//

#ifndef _JEventLoop_
#define _JEventLoop_

#include <sys/time.h>

#include <vector>
#include <string>
#include <utility>
#include <typeinfo>
#include <string.h>
#include <map>
#include <utility>
using std::vector;
using std::string;
using std::type_info;

#include <JANA/jerror.h>
#include <JANA/JObject.h>
#include <JANA/JException.h>
#include <JANA/JEvent.h>
#include <JANA/JThread.h>
#include <JANA/JFactory_base.h>
#include <JANA/JCalibration.h>
#include <JANA/JGeometry.h>
#include <JANA/JResourceManager.h>
#include <JANA/JStreamLog.h>

// The following is here just so we can use ROOT's THtml class to generate documentation.
#include "cint.h"


// Place everything in JANA namespace
namespace jana{


template<class T> class JFactory;
class JApplication;
class JEventProcessor;


class JEventLoop{
	public:
	
		friend class JApplication;
	
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
			double start_time;
			double end_time;
			data_source_t data_source;
		}call_stack_t;
		
		typedef struct{
			const char* factory_name;
			string tag;
			const char* filename;
			int line;
		}error_call_stack_t;

	                                   JEventLoop(JApplication *app); ///< Constructor
							   virtual ~JEventLoop(); ////< Destructor
				   virtual const char* className(void){return static_className();}
					static const char* static_className(void){return "JEventLoop";}
		
						 JApplication* GetJApplication(void) const {return app;} ///< Get pointer to the JApplication object
                                  void RefreshProcessorListFromJApplication(void); ///< Re-copy the list of JEventProcessors from JApplication
                      virtual jerror_t AddFactory(JFactory_base* factory); ///< Add a factory
							  jerror_t RemoveFactory(JFactory_base* factory); ///< Remove a factory
                        JFactory_base* GetFactory(const string data_name, const char *tag="", bool allow_deftag=true); ///< Get a specific factory pointer
                vector<JFactory_base*> GetFactories(void) const {return factories;} ///< Get all factory pointers
                                  void GetFactoryNames(vector<string> &factorynames); ///< Get names of all factories in name:tag format
                                  void GetFactoryNames(map<string,string> &factorynames); ///< Get names of all factories in map with key=name, value=tag
                    map<string,string> GetDefaultTags(void) const {return default_tags;}
                              jerror_t ClearFactories(void); ///< Reset all factories in preparation for next event.
							  jerror_t PrintFactories(int sparsify=0); ///< Print a list of all factories.
                              jerror_t Print(const string data_name, const char *tag=""); ///< Print the data of the given type

						 JCalibration* GetJCalibration();
                template<class T> bool GetCalib(string namepath, map<string,T> &vals);
                template<class T> bool GetCalib(string namepath, vector<T> &vals);

                            JGeometry* GetJGeometry();
                template<class T> bool GetGeom(string namepath, map<string,T> &vals);
			    template<class T> bool GetGeom(string namepath, T &val);

                     JResourceManager* GetJResourceManager(void);
                                string GetResource(string namepath);
                template<class T> bool GetResource(string namepath, T vals, int event_number=0);

                                  void Initialize(void); ///< Do initializations just before event processing starts
                              jerror_t Loop(void); ///< Loop over events
                              jerror_t OneEvent(void); ///< Process a single event
                           inline void Pause(void){pause = 1;} ///< Pause event processing
                           inline void Resume(void){pause = 0;} ///< Resume event processing
                           inline void Quit(void){quit = 1;} ///< Clean up and exit the event loop
                           inline bool GetQuit(void) const {return quit;}
                                  void QuitProgram(void);

        template<class T> JFactory<T>* GetSingle(const T* &t, const char *tag="", bool exception_if_not_one=true); ///< Get pointer to first data object from (source or factory).
        template<class T> JFactory<T>* Get(vector<const T*> &t, const char *tag="", bool allow_deftag=true); ///< Get data object pointers from (source or factory)
        template<class T> JFactory<T>* GetFromFactory(vector<const T*> &t, const char *tag="", data_source_t &data_source=null_data_source, bool allow_deftag=true); ///< Get data object pointers from factory
            template<class T> jerror_t GetFromSource(vector<const T*> &t, JFactory_base *factory=NULL); ///< Get data object pointers from source.
                        inline JEvent& GetJEvent(void){return event;} ///< Get pointer to the current JEvent object.
                           inline void SetJEvent(JEvent *event){this->event = *event;} ///< Set the JEvent pointer.
                           inline void SetAutoFree(int auto_free){this->auto_free = auto_free;} ///< Set the Auto-Free flag on/off
                      inline pthread_t GetPThreadID(void) const {return pthread_id;} ///< Get the pthread of the thread to which this JEventLoop belongs
                                double GetInstantaneousRate(void) const {return rate_instantaneous;} ///< Get the current event processing rate
                                double GetIntegratedRate(void) const {return rate_integrated;} ///< Get the current event processing rate
                                double GetLastEventProcessingTime(void) const {return delta_time_single;}
                          unsigned int GetNevents(void) const {return Nevents;}

                           inline bool CheckEventBoundary(uint64_t event_numberA, uint64_t event_numberB);

                           inline bool GetCallStackRecordingStatus(void){ return record_call_stack; }
                           inline void DisableCallStackRecording(void){ record_call_stack = false; }
                           inline void EnableCallStackRecording(void){ record_call_stack = true; }
           inline vector<call_stack_t> GetCallStack(void){return call_stack;} ///< Get the current factory call stack
                           inline void AddToCallStack(call_stack_t &cs){if(record_call_stack) call_stack.push_back(cs);} ///< Add specified item to call stack record but only if record_call_stack is true
                           inline void AddToErrorCallStack(error_call_stack_t &cs){error_call_stack.push_back(cs);} ///< Add layer to the factory call stack
     inline vector<error_call_stack_t> GetErrorCallStack(void){return error_call_stack;} ///< Get the current factory error call stack
                                  void PrintErrorCallStack(void); ///< Print the current factory call stack

                        const JObject* FindByID(JObject::oid_t id); ///< Find a data object by its identifier.
            template<class T> const T* FindByID(JObject::oid_t id); ///< Find a data object by its type and identifier
                        JFactory_base* FindOwner(const JObject *t); ///< Find the factory that owns a data object by pointer
                        JFactory_base* FindOwner(JObject::oid_t id); ///< Find a factory that owns a data object by identifier

                                       // User defined references
                template<class T> void SetRef(T *t);    ///< Add a user reference to this JEventLoop (must be a pointer)
                  template<class T> T* GetRef(void);   ///< Get a user-defined reference of a specific type
          template<class T> vector<T*> GetRefsT(void); ///< Get all user-defined refrences of a specicif type
     vector<pair<const char*, void*> > GetRefs(void){ return user_refs; }  ///< Get copy of full list of user-defined references
                template<class T> void RemoveRef(T *t); ///< Remove user reference from list

									   // Convenience methods wrapping JEvent methods of same name
		                      uint64_t GetStatus(void){return event.GetStatus();}
		                          bool GetStatusBit(uint32_t bit){return event.GetStatusBit(bit);}
		                          bool SetStatusBit(uint32_t bit, bool val=true){return event.SetStatusBit(bit, val);}
		                          bool ClearStatusBit(uint32_t bit){return event.ClearStatusBit(bit);}
		                          void ClearStatus(void){event.ClearStatus();}
		                          void SetStatusBitDescription(uint32_t bit, string description){event.SetStatusBitDescription(bit, description);}
		                        string GetStatusBitDescription(uint32_t bit){return event.GetStatusBitDescription(bit);}
		                          void GetStatusBitDescriptions(map<uint32_t, string> &status_bit_descriptions){return event.GetStatusBitDescriptions(status_bit_descriptions);}
                                string StatusWordToString(void);

	private:
		JEvent event;
		vector<JFactory_base*> factories;
		vector<JEventProcessor*> processors;
		vector<error_call_stack_t> error_call_stack;
		vector<call_stack_t> call_stack;
		JApplication *app;
		JThread *jthread;
		bool initialized;
		bool print_parameters_called;
		int pause;
		int quit;
		int auto_free;
		pthread_t pthread_id;
		map<string, string> default_tags;
		vector<pair<string,string> > auto_activated_factories;
		bool record_call_stack;
		string caller_name;
		string caller_tag;
		vector<uint64_t> event_boundaries;
		int32_t event_boundaries_run; ///< Run number boundaries were retrieved from (possbily 0)
		
		uint64_t Nevents;			      ///< Total events processed (this thread)
		uint64_t Nevents_rate;		   ///< Num. events accumulated for "instantaneous" rate
		double delta_time_single;		///< Time spent processing last event
		double delta_time_rate;			///< Integrated time accumulated "instantaneous" rate (partial number of events)
		double delta_time;				///< Total time spent processing events (this thread)
		double rate_instantaneous;		///< Latest instantaneous rate
		double rate_integrated;			///< Rate integrated over all events
   
		static data_source_t null_data_source;

		vector<pair<const char*, void*> > user_refs;
};


// The following is here just so we can use ROOT's THtml class to generate documentation.
#ifdef G__DICTIONARY
typedef JEventLoop::call_stack_t call_stack_t;
typedef JEventLoop::error_call_stack_t error_call_stack_t;
#endif

#if !defined(__CINT__) && !defined(__CLING__)

//-------------
// GetSingle
//-------------
template<class T>
JFactory<T>* JEventLoop::GetSingle(const T* &t, const char *tag, bool exception_if_not_one)
{
	/// This is a convenience method that can be used to get a pointer to the single
	/// object of type T from the specified factory. It simply calls the Get(vector<...>) method
	/// and copies the first pointer into "t" (or NULL if something other than 1 object is returned).
	/// 
	/// This is intended to address the common situation in which there is an interest
	/// in the event if and only if there is exactly 1 object of type T. If the event
	/// has no objects of that type or more than 1 object of that type (for the specified
	/// factory) then an exception of type "unsigned long" is thrown with the value
	/// being the number of objects of type T. You can supress the exception by setting
	/// exception_if_not_one to false. In that case, you will have to check if t==NULL to
	/// know if the call succeeded.
	vector<const T*> v;
	JFactory<T> *fac = Get(v, tag);

	if(v.size()!=1){
		t = NULL;
		if(exception_if_not_one) throw v.size();
	}
	
	t = v[0];
	
	return fac;
}

//-------------
// Get
//-------------
template<class T> 
JFactory<T>* JEventLoop::Get(vector<const T*> &t, const char *tag, bool allow_deftag)
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

	// Check if a tag was specified for this data type to use for the
	// default.
	const char *mytag = tag==NULL ? "":tag; // protection against NULL tags
	if(strlen(mytag)==0 && allow_deftag){
		map<string, string>::const_iterator iter = default_tags.find(T::static_className());
		if(iter!=default_tags.end())tag = iter->second.c_str();
	}
	
	
	// If we are trying to keep track of the call stack then we
	// need to add a new call_stack_t object to the the list
	// and initialize it with the start time and caller/callee
	// info.
	call_stack_t cs;
	if(record_call_stack){
		struct itimerval tmr;
		getitimer(ITIMER_PROF, &tmr);

		cs.caller_name = caller_name;
		cs.caller_tag = caller_tag;
		cs.callee_name = T::static_className();
		cs.callee_tag = tag;
		caller_name = cs.callee_name;
		caller_tag = cs.callee_tag;
		cs.start_time = tmr.it_value.tv_sec + tmr.it_value.tv_usec/1.0E6;
	}

	// Get the data (or at least try to)
	JFactory<T>* factory=NULL;
	try{
		factory = GetFromFactory(t, tag, cs.data_source, allow_deftag);
		if(!factory){
			// No factory exists for this type and tag. It's possible
			// that the source may be able to provide the objects
			// but it will need a place to put them. We can create a
			// dumb JFactory just to hold the data in case the source
			// can provide the objects. Before we do though, make sure
			// the user condones this via the presence of the
			// "JANA:AUTOFACTORYCREATE" config parameter.
			string p;
			try{
				gPARMS->GetParameter("JANA:AUTOFACTORYCREATE", p);
			}catch(...){}
			if(p.size()==0){
				jout<<std::endl;
				_DBG__;
				jout<<"No factory of type \""<<T::static_className()<<"\" with tag \""<<tag<<"\" exists."<<std::endl;
				jout<<"If you are reading objects from a file, I can auto-create a factory"<<std::endl;
				jout<<"of the appropriate type to hold the objects, but this feature is turned"<<std::endl;
				jout<<"off by default. To turn it on, set the \"JANA:AUTOFACTORYCREATE\""<<std::endl;
				jout<<"configuration parameter. This can usually be done by passing the"<<std::endl;
				jout<<"following argument to the program from the command line:"<<std::endl;
				jout<<std::endl;
				jout<<"   -PJANA:AUTOFACTORYCREATE=1"<<std::endl;
				jout<<std::endl;
				jout<<"Note that since the most commonly expected occurance of this situation."<<std::endl;
				jout<<"is an error, the program will now throw an exception so that the factory."<<std::endl;
				jout<<"call stack can be printed."<<std::endl;
				jout<<std::endl;
				throw exception();
			}else{
				AddFactory(new JFactory<T>(tag));
				jout<<__FILE__<<":"<<__LINE__<<" Auto-created "<<T::static_className()<<":"<<tag<<" factory"<<std::endl;
			
				// Now try once more. The GetFromFactory method will call
				// GetFromSource since it's empty.
				factory = GetFromFactory(t, tag, cs.data_source, allow_deftag);
			}
		}
	}catch(exception &e){
		// Uh-oh, an exception was thrown. Add us to the call stack
		// and re-throw the exception
		error_call_stack_t ecs;
		ecs.factory_name = T::static_className();
		ecs.tag = tag;
		ecs.filename = NULL;
		error_call_stack.push_back(ecs);
		throw e;
	}
	
	// If recording the call stack, update the end_time field
	if(record_call_stack){
		struct itimerval tmr;
		getitimer(ITIMER_PROF, &tmr);
		cs.end_time = tmr.it_value.tv_sec + tmr.it_value.tv_usec/1.0E6;
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
JFactory<T>* JEventLoop::GetFromFactory(vector<const T*> &t, const char *tag, data_source_t &data_source, bool allow_deftag)
{
	// We need to find the factory providing data type T with
	// tag given by "tag". 
	vector<JFactory_base*>::iterator iter=factories.begin();
	JFactory<T> *factory = NULL;
	string className(T::static_className());
	
	// Check if a tag was specified for this data type to use for the
	// default.
	const char *mytag = tag==NULL ? "":tag; // protection against NULL tags
	if(strlen(mytag)==0 && allow_deftag){
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
			// All we need to do now is just set the factory pointers in
			// the newly generated JObjects and return the factory pointer.

			factory->SetFactoryPointers();
			data_source = DATA_FROM_SOURCE;

			return factory;
		}
	}
		
	// OK. It looks like we have to have the factory make this.
	// Get pointers to data from the factory.
	factory->Get(t);
	factory->SetFactoryPointers();
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
// CheckEventBoundary
//-------------
inline bool JEventLoop::CheckEventBoundary(uint64_t event_numberA, uint64_t event_numberB)
{
	/// Check whether the two event numbers span one or more boundaries
	/// in the calibration/conditions database for the current run number.
	/// Return true if they do and false if they don't. The first parameter
	/// "event_numberA" is also checked if it lands on a boundary in which
	/// case true is also returned. If event_numberB lands on a boundary,
	/// but event_numberA does not, then false is returned.
	///
	/// This method is not expected to be called by a user. It is, however called,
	/// everytime a JFactory's Get() method is called.

	// Make sure our copy of the boundaries is up to date
	if(event.GetRunNumber()!=event_boundaries_run){
		event_boundaries.clear(); // in case we can't get the JCalibration pointer
		JCalibration *jcalib = GetJCalibration();
		if(jcalib)jcalib->GetEventBoundaries(event_boundaries);
		event_boundaries_run = event.GetRunNumber();
	}
	
	// Loop over boundaries
	for(unsigned int i=0; i<event_boundaries.size(); i++){
		uint64_t eb = event_boundaries[i];
		if((eb - event_numberA)*(eb - event_numberB) < 0.0 || eb==event_numberA){ // think about it ....
			// events span a boundary or is on a boundary. Return true
			return true;
		}
	}

	return false;
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
	for(unsigned int i=0; i<factories.size(); i++){
		if(factories[i]->GetDataClassName() != T::static_className())continue;

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
	
	return calib->Get(namepath, vals, event.GetEventNumber());
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
	
	return calib->Get(namepath, vals, event.GetEventNumber());
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

//-------------
// SetRef
//-------------
template<class T>
void JEventLoop::SetRef(T *t)
{
	pair<const char*, void*> p(typeid(T).name(), (void*)t);
	user_refs.push_back(p);
}

//-------------
// GetResource
//-------------
template<class T> bool JEventLoop::GetResource(string namepath, T vals, int event_number)
{
	JResourceManager *resource_manager = GetJResourceManager();
	if(!resource_manager){
		string mess = string("Unable to get the JResourceManager object (namepath=\"")+namepath+"\")";
		throw JException(mess);
	}

	return resource_manager->Get(namepath, vals, event_number);
}

//-------------
// GetRef
//-------------
template<class T>
T* JEventLoop::GetRef(void)
{
	/// Get a user-defined reference (a pointer)
	for(unsigned int i=0; i<user_refs.size(); i++){
		if(user_refs[i].first == typeid(T).name()) return (T*)user_refs[i].second;
	}

	return NULL;
}

//-------------
// GetRefsT
//-------------
template<class T>
vector<T*> JEventLoop::GetRefsT(void)
{
	vector<T*> refs;
	for(unsigned int i=0; i<user_refs.size(); i++){
		if(user_refs[i].first == typeid(T).name()){
			refs.push_back((T*)user_refs[i].second);
		}
	}

	return refs;
}

//-------------
// RemoveRef
//-------------
template<class T>
void JEventLoop::RemoveRef(T *t)
{
	vector<pair<const char*, void*> >::iterator iter;
	for(iter=user_refs.begin(); iter!= user_refs.end(); iter++){
		if(iter->second == (void*)t){
			user_refs.erase(iter);
			return;
		}
	}
	_DBG_<<" Attempt to remove user reference not in event loop!" << std::endl;
}


#endif //__CINT__  __CLING__

} // Close JANA namespace



#endif // _JEventLoop_

