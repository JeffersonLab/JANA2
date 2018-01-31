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
#ifndef _JEvent_h_
#define _JEvent_h_

#include <vector>
#include <cstddef>
#include <memory>

#include <JObject.h>
#include <JException.h>
#include <JFactorySet.h>
#include "JResettable.h"
#include "JFactory.h"
#include "JEventSource.h"
#include "JApplication.h"

class JEvent : public JResettable, std::enable_shared_from_this<JEvent>
{
	public:

		JEvent();
		virtual ~JEvent();
		
		//FACTORIES
		void SetFactorySet(const std::shared_ptr<JFactorySet>& aFactorySet);
		template<class DataType>
		JFactory<DataType>* GetFactory(const std::string& aTag = "") const;

		//OBJECTS
		template<class DataType>
		typename JFactory<DataType>::PairType Get(const std::string& aTag = "") const;

		//RESOURCES
		void Release(void);
		void Reset(void){}; //Called when acquired-from resource pool

		//SETTERS
		void SetRunNumber(uint32_t aRunNumber){mRunNumber = aRunNumber;}
		void SetEventNumber(uint64_t aEventNumber){mEventNumber = aEventNumber;}
		void SetEventSource(JEventSource* aSource);

		//GETTERS
		uint32_t GetRunNumber(void) const{return mRunNumber;}
		uint64_t GetEventNumber(void) const{return mEventNumber;}
		JEventSource* GetEventSource(void) const;

	protected:
	
		std::shared_ptr<JFactorySet> mFactorySet = nullptr;
		JEventSource* mEventSource = nullptr;

	private:
		uint32_t mRunNumber = 0;
		uint64_t mEventNumber = 0;
};

//---------------------------------
// GetFactory
//---------------------------------
template<class DataType>
inline JFactory<DataType>* JEvent::GetFactory(const std::string& aTag) const
{
	return mFactorySet->GetFactory(std::type_index(typeid(DataType)), aTag);
}

//---------------------------------
// Get
//---------------------------------
template<class DataType>
typename JFactory<DataType>::PairType JEvent::Get(const std::string& aTag) const
{
	//First get the factory
	auto sFactory = GetFactory<DataType>(aTag);
	if(sFactory == nullptr)
	{
		//Uh oh, No factory exists for this type.
		jerr << "ERROR: No factory found for type = " << typeid(DataType).name() << ", tag = " << aTag << "\n";
		japp->SetExitCode(-1);
		japp->Quit();
		//TODO: Throw exception??
		return {};
	}

	//If objects previously created, just return them
	if(sFactory->GetCreated())
		return sFactory->Get();

	//Attempt to acquire the "creating" lock for the factory
	//If we succeed, first check to see if they were created in between
		//Then create the objects and return them
	//If we fail, another thread is currently creating the objects.
		//Instead, execute queued tasks until the objects are ready
	if(!sFactory->AcquireCreatingLock())
	{

	}

	//Lock acquired.
	//TODO: Be careful! If we throw exception within here, we may not release the lock. Put try/catch loop
	//First though check to see whether another thread created the objects
	//between our last check of GetCreated and acquiring the lock.
	if(sFactory->GetCreated())
	{
		sFactory->ReleaseCreatingLock();
		return sFactory->Get();
	}

	//If not, first try to get from the event source
	auto sSharedThis = this->shared_from_this();
	if(mEventSource->GetObjects(sSharedThis, sFactory))
	{
		sFactory->SetCreated(true);
		sFactory->ReleaseCreatingLock();
		return sFactory->Get();
	}

	//Not in the file: have the factory make them
	//First compare current run # to previous run. If different, call ChangeRun()
	if(sFactory->GetPreviousRunNumber() != mRunNumber)
	{
		sFactory->ChangeRun(sSharedThis);
		sFactory->SetPreviousRunNumber(mRunNumber);
	}

	//Create the objects
	sFactory->Create(sSharedThis);
	sFactory->SetCreated(true);
	sFactory->ReleaseCreatingLock();

	//Return the objects
	return sFactory->Get();
}

#endif // _JEvent_h_

