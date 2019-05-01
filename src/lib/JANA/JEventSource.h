//
//    File: JEventSource.h
// Created: Thu Oct 12 08:15:39 EDT 2017
// Creator: davidl (on Darwin harriet.jlab.org 15.6.0 i386)
//
// ------ Last repository commit info -----
// [ Date ]
// [ Author ]
// [ Source ]
// [ Revision ]
//
//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - 
// Jefferson Science Associates LLC Copyright Notice:  
// Copyright 251 2014 Jefferson Science Associates LLC All Rights Reserved. Redistribution
// and use in source and binary forms, with or without modification, are permitted as a
// licensed user provided that the following conditions are met:  
// 1. Redistributions of source code must retain the above copyright notice, this
//    list of conditions and the following disclaimer. 
// 2. Redistributions in binary form must reproduce the above copyright notice, this
//    list of conditions and the following disclaimer in the documentation and/or other
//    materials provided with the distribution.  
// 3. The name of the author may not be used to endorse or promote products derived
//    from this software without specific prior written permission.  
// This material resulted from work developed under a United States Government Contract.
// The Government retains a paid-up, nonexclusive, irrevocable worldwide license in such
// copyrighted data to reproduce, distribute copies to the public, prepare derivative works,
// perform publicly and display publicly and to permit others to do so.   
// THIS SOFTWARE IS PROVIDED BY JEFFERSON SCIENCE ASSOCIATES LLC "AS IS" AND ANY EXPRESS
// OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
// MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL
// JEFFERSON SCIENCE ASSOCIATES, LLC OR THE U.S. GOVERNMENT BE LIABLE TO LICENSEE OR ANY
// THIRD PARTES FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
// DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS
// OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
// LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
// OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
// POSSIBILITY OF SUCH DAMAGE.
//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - 
//
// Description:
//
//
//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - 

#include <string>
#include <utility>
#include <atomic>
#include <memory>
#include <vector>
#include <typeindex>

#include <JANA/JApplication.h>
#include <JANA/JFactoryGenerator.h>

class JTaskBase;
class JQueue;
class JEvent;
class JEventSourceManager;
class JEventSourceGenerator;
class JFactory;
template<typename T> class JFactoryT;

//Deriving classes should:
//Overload all virtual methods (as needed)
//Create their own JFactoryGenerator and JQueue and store in the member variables (as needed)

#ifndef _JEventSource_h_
#define _JEventSource_h_

class JEventSource{

	friend JEvent;
	friend JEventSourceManager;
	friend JEventSourceGenerator;

	public:
	
		enum class RETURN_STATUS {
			kSUCCESS,
			kNO_MORE_EVENTS,
			kBUSY,
			kTRY_AGAIN,
			kERROR,
			kUNKNOWN
		};

		JEventSource(std::string name, JApplication* aApplication=nullptr);
		virtual ~JEventSource();
		
		virtual void Open(void);
		virtual bool GetObjects(const std::shared_ptr<const JEvent>& aEvent, JFactory* aFactory){return false;}

		void SetNumEventsToGetAtOnce(std::size_t aMinNumEvents, std::size_t aMaxNumEvents);
		std::pair<std::size_t, std::size_t> GetNumEventsToGetAtOnce(void) const; //returns min, max

		std::vector<std::shared_ptr<JTaskBase> > GetProcessEventTasks(std::size_t aNumTasks = 1);
		bool IsExhausted(void) const;
		std::size_t GetNumEventsProcessed(void) const;
		
		virtual std::string GetType(void) const; ///< Returns name of subclass
		virtual std::string GetVDescription(void) const {return "<description unavailable>";} ///< Optional for getting description via source rather than JEventSourceGenerator

		std::string GetName(void) const{return mName;}
		std::size_t GetNumOutstandingEvents(void) const{return mNumOutstandingEvents;}
		std::size_t GetNumOutstandingBarrierEvents(void) const{return mNumOutstandingBarrierEvents;}

		JQueue* GetEventQueue(void) const{return mEventQueue;}
		JFactoryGenerator* GetFactoryGenerator(void) const{return mFactoryGenerator;}
		std::type_index GetDerivedType(void) const {return std::type_index(typeid(*this));}

		void SetMaxEventsToRead(std::size_t aMaxEventsToRead){mMaxEventsToRead = aMaxEventsToRead;}
		std::size_t GetMaxEventsToRead(void){return mMaxEventsToRead;}

		std::atomic<std::size_t> mEventsRead{0};
		std::atomic<std::size_t> mTasksCreated{0};

	protected:
	
		void SetJApplication(JApplication *app);
		virtual void GetEvent(std::shared_ptr<JEvent>) = 0;
		JApplication* mApplication = nullptr;
		std::string mName;
		JQueue* mEventQueue = nullptr; //For handling event-source-specific logic (such as disentangling events, dealing with barriers, etc.)
		JFactoryGenerator* mFactoryGenerator = nullptr; //This should create default factories for all types available in the event source

	private:

		//Called by JEvent when recycling (decrement) and setting new event source (increment)
		void IncrementEventCount(void){mNumOutstandingEvents++;}
		void DecrementEventCount(void){mNumOutstandingEvents--; mEventsProcessed++;}
		void IncrementBarrierCount(void){mNumOutstandingBarrierEvents++;}
		void DecrementBarrierCount(void){mNumOutstandingBarrierEvents--;}

		virtual std::shared_ptr<JTaskBase> GetProcessEventTask(std::shared_ptr<const JEvent>&& aEvent);

		//Keep track of file/event status
		std::atomic<bool> mExhausted{false};
		std::atomic<bool> mGettingEvent{false};
		std::atomic<std::size_t> mEventsProcessed{0};
		std::atomic<std::size_t> mNumOutstandingEvents{0}; //Number of JEvents still be analyzed from this event source (source done when 0 (OR 1 and below is 1))
		std::atomic<std::size_t> mNumOutstandingBarrierEvents{0}; //Number of BARRIER JEvents still be analyzed from this event source (source done when 1)


		std::size_t mMinNumEventsToGetAtOnce = 1;
		std::size_t mMaxNumEventsToGetAtOnce = 1;
		std::size_t mMaxEventsToRead = 0;
};

#endif // _JEventSource_h_

