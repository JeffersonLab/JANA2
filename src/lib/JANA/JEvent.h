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


#include <JANA/JObject.h>
#include <JANA/JException.h>
#include <JANA/JFactoryT.h>
#include <JANA/JFactorySet.h>
#include <JANA/JEventSource.h>
#include <JANA/JLogger.h>

#include <JANA/Utils/JResettable.h>
#include <JANA/Utils/JTypeInfo.h>
#include <JANA/Utils/JCpuInfo.h>

#include <vector>
#include <cstddef>
#include <memory>
#include <exception>
#include <atomic>
#include <mutex>

class JApplication;

using std::vector;

class JEvent : public JResettable, public std::enable_shared_from_this<JEvent>
{
	public:

		explicit JEvent(JApplication* aApplication=nullptr) { mApplication = aApplication; }
		virtual ~JEvent() {
		    mFactorySet->Release();
		}

		//FACTORIES
		void SetFactorySet(JFactorySet* aFactorySet) { mFactorySet = aFactorySet; }
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

		//SETTERS
		void SetRunNumber(uint32_t aRunNumber){mRunNumber = aRunNumber;}
		void SetEventNumber(uint64_t aEventNumber){mEventNumber = aEventNumber;}
		void SetJApplication(JApplication* app){mApplication = app;}
		void SetJEventSource(JEventSource* aSource){mEventSource = aSource;}

		//GETTERS
		uint32_t GetRunNumber() const {return mRunNumber;}
		uint64_t GetEventNumber() const {return mEventNumber;}
		JApplication* GetJApplication() const {return mApplication;}
        JEventSource* GetJEventSource() const {return mEventSource; }
		friend class JEventPool;

	protected:
		mutable JFactorySet* mFactorySet = nullptr;
		JEventSource* mEventSource = nullptr;
		bool mIsBarrierEvent = false;

	private:
		JApplication* mApplication = nullptr;
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
		factory = new JFactoryT<T>(JTypeInfo::demangle<T>(), tag);
		mFactorySet->Add(factory);
	}
	factory->Insert(item);
}

template <class T>
inline void JEvent::Insert(const vector<T*>& items, const string& tag) const {

	auto factory = mFactorySet->GetFactory<T>(tag);
	if (factory == nullptr) {
		factory = new JFactoryT<T>(JTypeInfo::demangle<T>(), tag);
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
    auto factory = mFactorySet->GetFactory<T>(tag);
    if (factory == nullptr) {
        // If we couldn't find a factory, or if we found a factory
        // but it was a dummy and no data had been inserted
        // TODO: Better idea: each EventSource specifies which factories
        factory = new JFactoryT<T>(JTypeInfo::demangle<T>(), tag);

        if (mEventSource == nullptr || !(mEventSource->GetObjects(this->shared_from_this(), factory))) {
			throw JException("Could not find factory '" + factory->GetName() + "', tag=" + tag);
        }
	};
    return factory;
}


/// C-style getters

template<class T>
JFactoryT<T>* JEvent::Get(T** destination, const std::string& tag) const
{
	auto factory = GetFactory<T>(tag);
    auto iterators = factory->GetOrCreate(this->shared_from_this());
    if (std::distance(iterators.first, iterators.second) != 1) {
        throw JException("Wrong number of elements!");
    }
	*destination = *iterators.first;
	return factory;
}


template<class T>
JFactoryT<T>* JEvent::Get(vector<const T*>& destination, const std::string& tag) const
{
    auto factory = GetFactory<T>(tag);
    auto iterators = factory->GetOrCreate(this->shared_from_this());
    for (auto it=iterators.first; it!=iterators.second; it++) {
        destination.push_back(*it);
    }
    return factory;
}


/// C++ style getters

template<class T> const T* JEvent::GetSingle(const std::string& tag) const {
    auto iterators = GetFactory<T>(tag)->GetOrCreate(this->shared_from_this());
    if (std::distance(iterators.first, iterators.second) != 1) {
        throw JException("Wrong number of elements!");
    }
    return *iterators.first;
}


template<class T>
std::vector<const T*> JEvent::Get(const std::string& tag) const {

    auto iters = GetFactory<T>(tag)->GetOrCreate(this->shared_from_this());
    std::vector<const T*> vec;
    for (auto it=iters.first; it!=iters.second; ++it) {
        vec.push_back(*it);
    }
    return vec; // Assumes RVO
}


template<class T>
typename JFactoryT<T>::PairType JEvent::GetIterators(const std::string& tag) const {
    return GetFactory<T>(tag)->GetOrCreate(this->shared_from_this());
}

#endif // _JEvent_h_

