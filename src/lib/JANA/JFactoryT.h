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

#include <vector>

#include <JANA/JApplication.h>
#include <JANA/JFactory.h>
#include <JANA/JObject.h>
#include <JANA/JEventSource.h>
#include <JANA/Utils/JTypeInfo.h>

#ifndef _JFactoryT_h_
#define _JFactoryT_h_

template<typename T>
class JFactoryT : public JFactory {
public:

    using IteratorType = typename std::vector<T*>::const_iterator;
    using PairType = std::pair<IteratorType, IteratorType>;


    JFactoryT(const std::string& aName = JTypeInfo::demangle<T>(), const std::string& aTag = "")
    : JFactory(aName, aTag) {}

    ~JFactoryT() override = default;


    void Init() override {}
    void ChangeRun(const std::shared_ptr<const JEvent>& aEvent) override {}
    void Process(const std::shared_ptr<const JEvent>& aEvent) override {
        // TODO: Debate best thing to do in this case. Consider fa250WaveboardV1Hit
        LOG << "Dummy factory created but nothing was Inserted() or Set()." << LOG_END;
        //throw JException("Dummy factory created but nothing was Inserted() or Set().");
    }


    std::type_index GetObjectType(void) const override {
        return std::type_index(typeid(T));
    }

    /// GetOrCreate handles all the preconditions and postconditions involved in calling the user-defined Open(),
    /// ChangeRun(), and Process() methods. These include making sure the JFactory JApplication is set, Init() is called
    /// exactly once, exceptions are tagged with the originating plugin and eventsource, ChangeRun() is
    /// called if and only if the run number changes, etc.
    PairType GetOrCreate(const std::shared_ptr<const JEvent>& event, JApplication* app, uint64_t run_number) {

        //std::lock_guard<std::mutex> lock(mMutex);
        if (mApp == nullptr) {
            mApp = app;
        }
        switch (mStatus) {
            case Status::Uninitialized:
                try {
                    std::call_once(mInitFlag, &JFactory::Init, this);
                }
                catch (JException& ex) {
                    ex.plugin_name = mPluginName;
                    ex.component_name = mFactoryName;
                    throw ex;
                }
                catch (...) {
                    auto ex = JException("Unknown exception in JEventSource::Open()");
                    ex.nested_exception = std::current_exception();
                    ex.plugin_name = mPluginName;
                    ex.component_name = mFactoryName;
                    throw ex;
                }
            case Status::Unprocessed:
                if (mPreviousRunNumber != run_number) {
                    ChangeRun(event);
                    mPreviousRunNumber = run_number;
                }
                Process(event);
                mStatus = Status::Processed;
            case Status::Processed:
            case Status::Inserted:
                return std::make_pair(mData.cbegin(), mData.cend());
            default:
                throw JException("Enum is set to a garbage value somehow");
        }
    }

    /// Please use the typed setters instead whenever possible
    void Set(std::vector<JObject*>& aData) override {
        ClearData();
        for (auto jobj : aData) {
            T* casted = dynamic_cast<T*>(jobj);
            assert(casted != nullptr);
            mData.push_back(casted);
        }
    }

    /// Please use the typed setters instead whenever possible
    void Insert(JObject* aDatum) override {
        T* casted = dynamic_cast<T*>(aDatum);
        assert(casted != nullptr);
        mData.push_back(casted);
        mStatus = Status::Inserted;
        // TODO: assert correct mStatus precondition
    }

    void Set(const std::vector<T*>& aData) {
        ClearData();
        mData = aData;
        mStatus = Status::Inserted;
    }

    void Set(std::vector<T*>&& aData) {
        ClearData();
        mData = std::move(aData);
        mStatus = Status::Inserted;
    }

    void Insert(T* aDatum) {
        mData.push_back(aDatum);
        mStatus = Status::Inserted;
    }

    void ClearData() override {

        // ClearData() does nothing if persistent flag is set.
        // User must manually recycle data, e.g. during ChangeRun()
        if (TestFactoryFlag(JFactory_Flags_t::PERSISTENT)) {
            return;
        }

        // Assuming we _are_ the object owner, delete the underlying jobjects
        if (!TestFactoryFlag(JFactory_Flags_t::NOT_OBJECT_OWNER)) {
            for (auto p : mData) delete p;
        }
        mData.clear();
        mStatus = Status::Unprocessed;
    }

protected:

    std::vector<T*> mData;
};

#endif // _JFactoryT_h_

