// $Id: JFactory_base.h 1730 2006-05-02 17:02:57Z davidl $


#ifndef _JFACTORY_BASE_H_
#define _JFACTORY_BASE_H_

#include <vector>
#include <string>
using std::vector;
using std::string;

#include "JEventProcessor.h"

// The following is here just so we can use ROOT's THtml class to generate documentation.
#if defined(__CINT__) || defined(__CLING__)
class pthread_mutex_t;
typedef unsigned long pthread_t;
typedef unsigned long oid_t;
#endif


// Place everything in JANA namespace
namespace jana{

class JEventLoop;

/// This class is used as a base class for all factory classes.
/// Typically, a factory object will be an instance of the
/// the JFactory template class, (using the class type of the
/// objects it provides). In order for the JEvent object to
/// treat all factories alike (i.e. keep an array of them) they
/// must all be derived from a common base class. This is that
/// base class.
///
/// Note that most JANA users will not use this class
/// directly (with the possible exception of calling GetDataClassName()).
/// They may occasionally need to dynamic_cast a pointer
/// to a JFactory_base object into a pointer to the subclass
/// though.

class JFactory_base:public JEventProcessor{
	
	
	friend class JEvent;

	public:
	
		/// Get pointers to factories objects (as void*). 
		///
		/// This gets typecast in the template member function
		/// also named "Get()" in the JEventLoop class. Since there
		/// is no base class for vector objects, we give it something
		/// that should at least be the same size i.e. vector<void*>
		virtual vector<void*>& Get(void)=0;
		
		/// Returns the number of rows.
		virtual int GetNrows(bool force_call_to_get=false, bool do_not_call_get=false)=0;
		
		/// Returns the number of requests for this factory's data.
		int GetNcalls(void){return Ncalls_to_Get;}
		
		/// Returns the number of events this factory had to generate data for.
		int GetNgencalls(void){return Ncalls_to_evnt;}
		
		/// Delete the factory's data depending on the flags
		virtual jerror_t Reset(void)=0;
		
		/// Delete the factory's data regardless of the flags
		virtual jerror_t HardReset(void)=0;
		
		/// Return the pointer to the class's name which this
		/// factory provides.
		virtual const char* GetDataClassName(void)=0;
		
		/// Returns the size of the data class on which this factory is based
		virtual int GetDataClassSize(void)=0;
		
		/// Returns a string object with a nicely formatted ASCII table of the data
		string toString(void) const;
		virtual void toStrings(vector<vector<pair<string,string> > > &items, bool append_types=false) const =0;

		/// The data tag string associated with this factory. Most factories
		/// will not overide this.
		virtual inline const char* Tag(void){return "";}
		
		/// Find object pointer in factory's _data vector and return it
		virtual const JObject* GetByID(JObject::oid_t id)=0;

		/// Copy given object pointers into JFactory class (uses dynamic_cast to guarantee correct type)
		virtual jerror_t CopyTo(vector<JObject*> &data)=0;

		/// Used by JEventLoop to give a pointer back to itself
		void SetJEventLoop(JEventLoop *loop){this->eventLoop=loop;}
		
		enum JFactory_Flags_t{
			JFACTORY_NULL		=0x00,
			PERSISTANT			=0x01,
			WRITE_TO_OUTPUT	=0x02,
			NOT_OBJECT_OWNER	=0x04
		};
		
		/// Get all flags in the form of a single word
		inline unsigned int GetFactoryFlags(void){return flags;}
		
		/// Set a flag (or flags)
		inline void SetFactoryFlag(JFactory_Flags_t f){
			flags |= (unsigned int)f;
		}

		/// Clear a flag (or flags)
		inline void ClearFactoryFlag(JFactory_Flags_t f){
			flags &= ~(unsigned int)f;
		}

		/// Test if a flag (or set of flags) is set
		inline bool TestFactoryFlag(JFactory_Flags_t f){
			return (flags & (unsigned int)f) == (unsigned int)f;
		}
		
	
	protected:
		JEventLoop *eventLoop;
		unsigned int flags;
		int debug_level;
		int busy;
		unsigned int Ncalls_to_Get;
		unsigned int Ncalls_to_evnt;

};


} // Close JANA namespace

#endif // _JFACTORY_BASE_H_
