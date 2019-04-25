//
//    File: JEvent.h
// Created: Sun Oct 15 21:15:05 CDT 2017
// Creator: davidl (on Darwin harriet.local 15.6.0 i386)
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

#include <vector>
#include <cstddef>
#include <memory>
#include <exception>
#include <atomic>
#include <mutex>

#include <JANA/JObject.h>
#include <JANA/JException.h>
#include <JANA/JFactoryT.h>
#include <JANA/JFactorySet.h>
#include <JANA/JResettable.h>
#include <JANA/JEventSource.h>
#include <JANA/JFunctions.h>
#include <JANA/JApplication.h>
#include <JANA/JThreadManager.h>
#include <JANA/JLogger.h>
#include <JANA/JThread.h>

#ifndef _JEvent_h_
#define _JEvent_h_

class JEvent : public JResettable, public std::enable_shared_from_this<JEvent>
{
	public:

		JEvent(JApplication* aApplication=nullptr);
		virtual ~JEvent();

		//FACTORIES
		void SetFactorySet(JFactorySet* aFactorySet);
		template<class T> JFactoryT<T>* GetFactory(const std::string& tag = "") const;

		//OBJECTS
		// C style getters
		template<class T> JFactoryT<T>* Get(T** item, const std::string& tag="") const;
		template<class T> JFactoryT<T>* Get(vector<const T*> &vec, const std::string& tag = "") const;

		// C++ style getters
		template<class T> const T* GetSingle(const std::string& tag = "") const;
		template<class T> vector<const T*> Get(const std::string& tag = "") const;
		template<class T> typename JFactoryT<T>::PairType GetIterators(const std::string& aTag = "") const;

		// Insert
		template <class T> void Insert(T* item, const std::string& aTag = "") const;
		template <class T> void Insert(const std::vector<T*>& items, const std::string& tag = "") const;

		//RESOURCES
		void Release(void);

		//SETTERS
		void SetRunNumber(uint32_t aRunNumber){mRunNumber = aRunNumber;}
		void SetEventNumber(uint64_t aEventNumber){mEventNumber = aEventNumber;}
		void SetIsBarrierEvent(bool aIsBarrierEvent=true){mIsBarrierEvent = aIsBarrierEvent;}
		void SetJApplication(JApplication* app);
		void SetJEventSource(JEventSource* aSource);
		void SetLatestBarrierEvent(const std::shared_ptr<const JEvent>& aBarrierEvent){mLatestBarrierEvent = aBarrierEvent;}

		//GETTERS
		uint32_t GetRunNumber(void) const{return mRunNumber;}
		uint64_t GetEventNumber(void) const{return mEventNumber;}
		JEventSource* GetEventSource(void) const;
		bool GetIsBarrierEvent(void) const{return mIsBarrierEvent;}
		JApplication* GetJApplication(void) const {return mApplication;}

	protected:
		mutable JFactorySet* mFactorySet = nullptr;
		JEventSource* mEventSource = nullptr;
		bool mIsBarrierEvent = false;

	private:
		JApplication* mApplication = nullptr;
		JThreadManager* mThreadManager = nullptr;
		uint32_t mRunNumber = 0;
		uint64_t mEventNumber = 0;
		int mDebugLevel = 0;
		std::shared_ptr<const JEvent> mLatestBarrierEvent = nullptr;
};

/// Insert() allows an EventSource to insert items directly into the JEvent,
/// removing the need for user-extended JEvents and/or JEventSource::GetObjects(...)
/// Repeated calls to Insert() will append to the previous data rather than overwrite it,
/// which saves the user from having to allocate a throwaway vector and requires less error handling.
template <class T>
inline void JEvent::Insert(T* item, const string& tag) const {

	auto factory = mFactorySet->GetFactory<T>(tag);
	if (factory == nullptr) {
		factory = new JFactoryT<T>(GetDemangledName<T>(), tag);
		factory->SetCreated(true);
		mFactorySet->Add(factory);
	}
	factory->Insert(item);
}

template <class T>
inline void JEvent::Insert(const vector<T*>& items, const string& tag) const {

	auto factory = mFactorySet->GetFactory<T>(tag);
	if (factory == nullptr) {
		factory = new JFactoryT<T>(GetDemangledName<T>(), tag);
		factory->SetCreated(true);
		mFactorySet->Add(factory);
	}
	for (T* item : items) {
		factory->Insert(item);
	}
}

//---------------------------------
// GetFactory
//---------------------------------
template<class T>
inline JFactoryT<T>* JEvent::GetFactory(const std::string& tag) const
{
	return mFactorySet->GetFactory<T>(tag);
}


/// C-style getters

template<class T>
JFactoryT<T>* JEvent::Get(T** destination, const std::string& tag) const
{
	auto factory = GetFactory<T>(tag);
	auto iterators = GetIterators<T>(tag);
	*destination = *iterators.first;
	return factory;
}

template<class T>
JFactoryT<T>* JEvent::Get(vector<const T*>& destination, const std::string& tag) const
{
	auto iterators = GetIterators<T>(tag);

	for (auto it=iterators.first; it!=iterators.second; it++) {
		destination.push_back(*it);
	}
	return GetFactory<T>(tag);
}


/// C++ style getters

template<class T> const T* JEvent::GetSingle(const std::string& tag) const {
	auto result = GetIterators<T>(tag);
	return *result.first;
}

template<class T>
vector<const T*> JEvent::Get(const std::string& aTag) const
{
	auto pt = GetIterators<T>( aTag );

	vector<const T*> vec;
	for(auto it=pt.first; it!=pt.second; it++) vec.push_back( *it );

	return vec; // should get moved by Return Value Optimization
}

template<class DataType>
typename JFactoryT<DataType>::PairType JEvent::GetIterators(const std::string& aTag) const
{
	if(mDebugLevel > 0)
		JLog() << "Thread " << JTHREAD->GetThreadID() << " JEvent::Get(): Type = " << GetDemangledName<DataType>() << ", tag = " << aTag << ".\n" << JLogEnd();

	//--------------------------------------------------------------------------------------------
	// Something is amiss below. It looks like the following block was intended to
	// pull objects from a previous barrier event. However, it does not actually
	// access the mLatestBarrierEvent (excpet to check that it is not nullptr)
	// The code block after this is identical which means this is redundant.
	// This may have been something Paul was working on but didn't quite complete.
	// I'm commenting it out for now as the handling of barrier event data
	// is better fleshed out.

// 	//First check to see if the information should come from the previous barrier event
// 	if(!mIsBarrierEvent && (mLatestBarrierEvent != nullptr))
// 	{
// 		//Get the factory
// 		auto sFactoryBase = mFactorySet->GetFactory(std::type_index(typeid(DataType)), aTag);
// 		if(sFactoryBase == nullptr)
// 		{
// 			//Uh oh, No factory exists for this type.
// 			jerr << "ERROR: No factory found for type = " << GetDemangledName<DataType>() << ", tag = \"" << aTag << "\\n";
// 			japp->SetExitCode(-1);
// 			japp->Quit();
// 			return {};
// 		}
// 		auto sFactory = static_cast<JFactoryT<DataType>*>(sFactoryBase);
// 
// 		//If the factory created the objects, then yes, use the event barrier data.
// 		//If not, the information must come from the current event instead.
// 		if(sFactory->GetCreated())
// 			return sFactory->Get();
// 	}
	//--------------------------------------------------------------------------------------------


	// Nathan says: Shouldn't most of the code below live in JFactorySet, so that JEvent.Get() proxies JFactorySet.Get()?
	//              JFactorySet isn't providing very meaningful abstraction; its internals are leaking out all over here.
	//              Alternatively, we could merge JEvent and JFactorySet, since instances of the two will have a solidly
	//              one-to-one relationship once we move away from user-defined JEvents.
	//              (Unless we wanted to use JFactorySets for subevent parallelism as well...)

	assert(mFactorySet != nullptr);

	auto sFactoryBase = mFactorySet->GetFactory(std::type_index(typeid(DataType)), aTag);
	if(sFactoryBase == nullptr)
	{
		//Uh oh, No factory exists for this type.
		jerr << "ERROR: No factory found for type = " << GetDemangledName<DataType>() << ", tag = \"" << aTag << "\"\n";
		japp->SetExitCode(-1);
		japp->Quit();
		return {};
	}
	auto sFactory = static_cast<JFactoryT<DataType>*>(sFactoryBase);

	//If objects previously created, just return them
	if(sFactory->GetCreated()) return sFactory->Get();

	// Objects are not already created so we may need to create them.
	// Ensure the Init method has been called for the factory.
	std::call_once(sFactory->init_flag, &JFactory::Init, sFactory);

	//Attempt to acquire the "creating" lock for the factory
	if(!sFactory->AcquireCreatingLock())
	{
		//We failed: Another thread is currently creating the objects.
		//Instead, execute queued tasks until the objects are ready

		if(mDebugLevel >= 10)
			JLog() << "Thread " << JTHREAD->GetThreadID() << " JEvent::Get(): Another thread is creating objects, do work while waiting.\n" << JLogEnd();

		// mThreadManager is set either in constructor or in SetJApplication.
		// It actually should always be set by the latter which is called from
		// JEventSource::GetProcessEventTasks after calling GetEvent.
		if (mThreadManager != nullptr) {
			mThreadManager->DoWorkWhileWaiting(sFactory->GetCreatingLock(), mEventSource);

			//It's done, return the results
			return sFactory->Get();
		}
		else {
			throw JException("Race condition: Multiple threads are attempting to JEvent::Get() the same object");
		}
	}

	if(mDebugLevel >= 10)
		JLog() << "Thread " << JTHREAD->GetThreadID() << " JEvent::Get(): Create lock acquired.\n" << JLogEnd();

	//If we throw exception within here we may not release the lock, unless we do try/catch:
	try
	{
		//Lock acquired. First check to see if they were created since the last check.
		if(sFactory->GetCreated())
		{
			if(mDebugLevel >= 10)
				JLog() << "Thread " << JTHREAD->GetThreadID() << " JEvent::Get(): Objects created in meantime.\n" << JLogEnd();
			sFactory->ReleaseCreatingLock();
			return sFactory->Get();
		}

		//Not yet: We need to create the objects.
		//First try to get from the event source
		if(mDebugLevel >= 10)
			JLog() << "Thread " << JTHREAD->GetThreadID() << " JEvent::Get(): Try to get " << GetDemangledName<DataType>() << " (tag = " << aTag << ") objects from JEventSource.\n" << JLogEnd();
		auto sSharedThis = this->shared_from_this();
		if (mEventSource != nullptr &&
			mEventSource->GetObjects(sSharedThis, static_cast<JFactory*>(sFactory)))
		{
			if(mDebugLevel >= 10)
				JLog() << "Thread " << JTHREAD->GetThreadID() << " JEvent::Get(): " << GetDemangledName<DataType>() << " (tag = " << aTag << ") retrieved from JEventSource.\n" << JLogEnd();
			sFactory->SetCreated(true);
			sFactory->ReleaseCreatingLock();
			return sFactory->Get();
		}

		//Not in the file: have the factory make them
		//First compare current run # to previous run. If different, call ChangeRun()
		if(sFactory->GetPreviousRunNumber() != mRunNumber)
		{
			if(mDebugLevel >= 10)
				JLog() << "Thread " << JTHREAD->GetThreadID() << " JEvent::Get(): Change run.\n" << JLogEnd();
			sFactory->ChangeRun(sSharedThis);
			sFactory->SetPreviousRunNumber(mRunNumber);
		}

		//Create the objects
		if(mDebugLevel >= 10)
			JLog() << "Thread " << JTHREAD->GetThreadID() << " JEvent::Get(): Create " << GetDemangledName<DataType>() << " (tag = " << aTag << ") with factory.\n" << JLogEnd();
		sFactory->Process(sSharedThis);
		sFactory->SetCreated(true);
		sFactory->ReleaseCreatingLock();
	}
	catch(...)
	{
		//Catch the exception, unlock, rethrow
		auto sException = std::current_exception();
		sFactory->ReleaseCreatingLock();
		std::rethrow_exception(sException);
	}

	//Get the object iterators
	auto sIteratorPair = sFactory->Get();
	if(mDebugLevel > 0)
		JLog() << "Thread " << JTHREAD->GetThreadID() << " JEvent::Get(): Getting " << std::distance(sIteratorPair.first, sIteratorPair.second) << " " << GetDemangledName<DataType>() << " objects, tag = " << aTag << ".\n" << JLogEnd();

	//Return the objects
	return sIteratorPair;
}

#endif // _JEvent_h_

