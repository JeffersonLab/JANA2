//
//    File: JEventProcessor.h
// Created: Thu Oct 12 08:15:32 EDT 2017
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

#ifndef _JEventProcessor_h_
#define _JEventProcessor_h_

#include <JANA/JEvent.h>
#include <JANA/Utils/JTypeInfo.h>
#include <atomic>

class JApplication;


class JEventProcessor {
public:

    explicit JEventProcessor(JApplication* app = nullptr)
        : mApplication(app)
        , m_status(Status::Unopened)
        , m_event_count{0}
    {}

    virtual ~JEventProcessor() = default;

    enum class Status { Unopened, Opened, Finished };

    Status GetStatus() const { return m_status; }

    std::string GetPluginName() const { return m_plugin_name; }

    std::string GetTypeName() const { return m_type_name; }

    std::string GetResourceName() const { return m_resource_name; }

    uint64_t GetEventCount() const { return m_event_count; };

    JApplication* GetApplication() const { return mApplication; }

    bool AreEventsOrdered() const { return m_receive_events_in_order; }


    virtual void DoInitialize() {
        try {
            std::call_once(m_init_flag, &JEventProcessor::Init, this);
            m_status = Status::Opened;
        }
        catch (JException& ex) {
            ex.plugin_name = m_plugin_name;
            ex.component_name = GetType();
            throw ex;
        }
        catch (...) {
            auto ex = JException("Unknown exception in JEventProcessor::Open()");
            ex.nested_exception = std::current_exception();
            ex.plugin_name = m_plugin_name;
            ex.component_name = GetType();
            throw ex;
        }
    }


    // TODO: Improve this type signature
    virtual void DoMap(const std::shared_ptr<const JEvent>& e) {
        try {
            std::call_once(m_init_flag, &JEventProcessor::DoInitialize, this);
            Process(e);
        }
        catch (JException& ex) {
            ex.plugin_name = m_plugin_name;
            ex.component_name = GetType();
            throw ex;
        }
        catch (...) {
            auto ex = JException("Unknown exception in JEventProcessor::DoMap()");
            ex.nested_exception = std::current_exception();
            ex.plugin_name = m_plugin_name;
            ex.component_name = GetType();
            throw ex;
        }
    }


    // Reduce does nothing in the basic version because the current API tells
    // the user to lock a mutex in Process(), which takes care of it for us.
    virtual void DoReduce(const std::shared_ptr<const JEvent>& e) {}


    virtual void DoFinalize() {
        try {
            std::call_once(m_finish_flag, &JEventProcessor::Finish, this);
            m_status = Status::Finished;
        }
        catch (JException& ex) {
            ex.plugin_name = m_plugin_name;
            ex.component_name = GetType();
            throw ex;
        }
        catch (...) {
            auto ex = JException("Unknown exception in JEventProcessor::Finish()");
            ex.nested_exception = std::current_exception();
            ex.plugin_name = m_plugin_name;
            ex.component_name = GetType();
            throw ex;
        }
    }


    // TODO: Make these protected?
    /// JEventProcessor::Init, Process, and Finish are meant to be written by the user.
    /// Each JEventProcessor is intended to generate one distinct output,
    virtual void Init() {}

    /// Process i
    virtual void Process(const std::shared_ptr<const JEvent>& aEvent) {
        throw JException("Not implemented yet!");
    }

    virtual void Finish() {}


    // TODO: Can we please deprecate this in favor of GetTypeName?
    virtual std::string GetType() const {
        return m_type_name;
    }


protected:

    // The following are meant to be called by the user from the constructor in order to
    // configure their JEventProcessor instance.

    /// SetTypeName is intended as a replacement to GetType(), which should be less confusing for the
    /// user. It should be called from the constructor. For convenience, we provide a
    /// NAME_OF_THIS macro so that the user doesn't have to type the class name as a string, which may
    /// get out of sync if automatic refactoring tools are used.

    void SetTypeName(std::string type_name) { m_type_name = std::move(type_name); }

    /// Resource name lets the user tell the parallelization engine to synchronize different EventProcessors
    /// which write to the same shared resource; e.g. if you have two EventProcessors
    /// which both write to a ROOT tree, they should both set the resource name 'ROOT'. On the flip side,
    /// if you have two EventProcessors which write to different resources, e.g. ROOT and a CSV file, and
    /// you set different resource names, the parallelization engine will know that it is safe to pipeline
    /// these two processors. If you don't set a resource name at all, the parallelization engine will
    /// assume that you are manually synchronizing access via your own mutex, which will be safe if and only
    /// if you use your locks correctly, and also may result in a performance penalty.

    void SetResourceName(std::string resource_name) { m_resource_name = std::move(resource_name); }

    /// SetEventsOrdered allows the user to tell the parallelization engine that it needs to see
    /// the event stream ordered by increasing event IDs. (Note that this requires all EventSources
    /// emit event IDs which are consecutive.) Ordering by event ID makes for cleaner output, but comes
    /// with a performance penalty, so it is best if this is enabled during debugging, and disabled otherwise.

    void SetEventsOrdered(bool receive_events_in_order) { m_receive_events_in_order = receive_events_in_order; }

    // TODO: Stop getting mApplication this way, use GetApplication() instead, or pass directly to Init()
    JApplication* mApplication = nullptr;

private:
    Status m_status;
    std::string m_plugin_name;
    std::string m_type_name;
    std::string m_resource_name;
    std::once_flag m_init_flag;
    std::once_flag m_finish_flag;
    std::atomic_ullong m_event_count;
    bool m_receive_events_in_order = false;

    /// This is called by JApplication::Add(JEventProcessor*). There
    /// should be no need to call it from anywhere else.
    void SetJApplication(JApplication* app) { mApplication = app; }

    friend JComponentManager;
    /// SetPlugin is called by ComponentManager and should not be exposed to the user.
    // TODO: Maybe we want JApplication to track the current plugin instead
    void SetPluginName(std::string plugin_name) { m_plugin_name = std::move(plugin_name); };

};

#endif // _JEventProcessor_h_

