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
#include <JANA/JLog.h>
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
		template<class DataType>
		JFactory* GetFactory(const std::string& aTag = "") const;

		//OBJECTS
		template<class T> JFactory* Get(vector<const T*> &vec, const std::string& aTag = "") const;
		template<class T> vector<const T*> GetT(const std::string& aTag = "") const;
		template<class T> typename JFactoryT<T>::PairType Get(const std::string& aTag = "") const;

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
	
		const JFactorySet* mFactorySet = nullptr;
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

//---------------------------------
// GetFactory
//---------------------------------
template<class DataType>
inline JFactory* JEvent::GetFactory(const std::string& aTag) const
{
	return mFactorySet->GetFactory(std::type_index(typeid(DataType)), aTag);
}

//---------------------------------
// Get
//---------------------------------
template<class T>
JFactory* JEvent::Get(vector<const T*> &vec, const std::string& aTag) const
{
	// Temporary wrapper for method below. The method below returns a beginning and
	// ending iterator indicating the objects. This was a design change by Paul that
	// I'm not sure I'm happy with, though I can see what he was trying to do and
	// understand the appeal. I'm just too lazy at the moment to change the whole
	// thing.

	auto pt = Get<T>( aTag );
	for(auto it=pt.first; it!=pt.second; it++) vec.push_back( *it );
	
	return const_cast<JFactory*>( GetFactory<T>( aTag ) );
}

//---------------------------------
// GetT
//---------------------------------
template<class T>
vector<const T*> JEvent::GetT(const std::string& aTag) const
{
	auto pt = Get<T>( aTag );
	
	vector<const T*> vec;
	for(auto it=pt.first; it!=pt.second; it++) vec.push_back( *it );
	
	return vec; // should get moved by Return Value Optimization
}

//---------------------------------
// Get
//---------------------------------
template<class DataType>
typename JFactoryT<DataType>::PairType JEvent::Get(const std::string& aTag) const
{
	// mThreadManager is set either in constructor or in SetJApplication. 
	// It actually should always be set by the latter which is called from
	// JEventSource::GetProcessEventTasks after calling GetEvent.
	assert(mThreadManager!=nullptr);

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

	//First get the factory
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
		if(mDebugLevel >= 10)
			JLog() << "Thread " << JTHREAD->GetThreadID() << " JEvent::Get(): Another thread is creating objects, do work while waiting.\n" << JLogEnd();

		//We failed: Another thread is currently creating the objects.
		//Instead, execute queued tasks until the objects are ready
		mThreadManager->DoWorkWhileWaiting(sFactory->GetCreatingLock(), mEventSource);

		//It's done, return the results
		return sFactory->Get();
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
		if(mEventSource->GetObjects(sSharedThis, static_cast<JFactory*>(sFactory)))
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

