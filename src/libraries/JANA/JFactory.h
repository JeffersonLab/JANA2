//
//    File: JFactory.h
// Created: Fri Oct 20 09:44:48 EDT 2017
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

#ifndef _JFactory_h_
#define _JFactory_h_

#include <JANA/JException.h>

#include <string>
#include <typeindex>
#include <memory>
#include <limits>
#include <atomic>
#include <vector>
#include <mutex>
#include <unordered_map>
#include <functional>


class JEvent;
class JObject;
class JApplication;

class JFactory {
public:

    enum JFactory_Flags_t {
        JFACTORY_NULL = 0x00,
        PERSISTENT = 0x01,
        WRITE_TO_OUTPUT = 0x02,
        NOT_OBJECT_OWNER = 0x04
    };

    JFactory(std::string aName, std::string aTag = "")
    : mObjectName(std::move(aName)), mTag(std::move(aTag)), mStatus(Status::Uninitialized) {};

    virtual ~JFactory() = default;

    std::string GetName() const { return mObjectName; }   // TODO: This is the JObject class name, right?

    std::string GetTag() const { return mTag; }

    uint32_t GetPreviousRunNumber(void) const { return mPreviousRunNumber; }

    void SetPreviousRunNumber(uint32_t aRunNumber) { mPreviousRunNumber = aRunNumber; }

    /// Get all flags in the form of a single word
    inline uint32_t GetFactoryFlags(void) { return mFlags; }

    /// Set a flag (or flags)
    inline void SetFactoryFlag(JFactory_Flags_t f) {
        mFlags |= (uint32_t) f;
    }

    /// Clear a flag (or flags)
    inline void ClearFactoryFlag(JFactory_Flags_t f) {
        mFlags &= ~(uint32_t) f;
    }

    /// Test if a flag (or set of flags) is set
    inline bool TestFactoryFlag(JFactory_Flags_t f) {
        return (mFlags & (uint32_t) f) == (uint32_t) f;
    }



    // Overloaded by JFactoryT
    virtual std::type_index GetObjectType() const = 0;

    virtual void ClearData() = 0;



    // Overloaded by user Factories
    virtual void Init() {}

    virtual void ChangeRun(const std::shared_ptr<const JEvent> &aEvent) {}

    virtual void Process(const std::shared_ptr<const JEvent> &aEvent) {}



    // Copy/Move objects into factory
    template<typename T>
    void Set(std::vector<T *> &items) {
        for (T *item : items) {
            Insert(item);
        }
    }

    /// Access the encapsulated data, performing an upcast if necessary. This is useful for extracting data from
    /// all JFactories<T> where T extends a parent class S, such as JObject or TObject, in contexts where T is not known
    /// or it would introduce an unwanted coupling. The main application is for building DSTs.
    ///
    /// Be aware of the following caveats:
    /// - The factory's object type must not use virtual inheritance.
    /// - If JFactory::Process hasn't already been called, this will return an empty vector. This will NOT call JFactory::Process.
    /// - Someone must call JFactoryT<T>::EnableGetAs<S>, preferably the constructor. Otherwise, this will return an empty vector.
    /// - If S isn't a base class of T, this will return an empty vector.
    template<typename S>
    std::vector<S*> GetAs();


    /// JApplication setter. This is meant to be used under the hood.
    void SetApplication(JApplication* app) { mApp = app; }

    /// JApplication getter. This is meant to be called by user-defined JFactories which need to
    /// acquire parameter values or services from JFactory::Init()
    JApplication* GetApplication() { return mApp; }


protected:
    virtual void Set(std::vector<JObject *> &data) = 0;

    virtual void Insert(JObject *data) = 0;

    std::string mPluginName;
    std::string mFactoryName;
    std::string mObjectName;
    std::string mTag;
    uint32_t mFlags = 0;
    uint32_t mPreviousRunNumber = -1;
    JApplication* mApp = nullptr;
    std::unordered_map<std::type_index, void*> mUpcastVTable;

    enum class Status {Uninitialized, Unprocessed, Processed, Inserted};
    mutable Status mStatus = Status::Uninitialized;
    mutable std::mutex mMutex;

    // Used to make sure Init is called only once
    std::once_flag mInitFlag;
};

// Because C++ doesn't support templated virtual functions, we implement our own dispatch table, mUpcastVTable.
// This means that the JFactoryT is forced to manually populate this table by calling JFactoryT<T>::EnableGetAs.
// We have the option to make the vtable be a static member of JFactoryT<T>, but we have chosen not to because:
//
//   1. It would be inconsistent with the fact that the user is supposed to call EnableGetAs in the ctor
//   2. People in the future may want to generalize GetAs to support user-defined S* -> T* conversions (which I don't recommend)
//   3. The size of the vtable is expected to be very small (<10 elements, most likely 2)

template<typename S>
std::vector<S*> JFactory::GetAs() {
    std::vector<S*> results;
    auto ti = std::type_index(typeid(S));
    auto search = mUpcastVTable.find(ti);
    if (search != mUpcastVTable.end()) {
        using upcast_fn_t = std::function<std::vector<S*>()>;
        auto upcast_fn = reinterpret_cast<upcast_fn_t*>(search->second);
        results = (*upcast_fn)();
    }
    return results;
}

#endif // _JFactory_h_

