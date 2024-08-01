
// Copyright 2020, Jefferson Science Associates, LLC.
// Subject to the terms in the LICENSE file found in the top-level directory.

#pragma once

#include <JANA/Omni/JComponent.h>
#include <JANA/Omni/JHasOutputs.h>
#include <JANA/JEvent.h>
#include <JANA/JException.h>
#include <JANA/JFactoryGenerator.h>


class JFactoryGenerator;
class JApplication;
class JFactory;


class JEventSource : public jana::omni::JComponent, public jana::omni::JHasOutputs {

public:
    
    /// Result describes what happened the last time a GetEvent() was attempted.
    /// If Emit() or GetEvent() reaches an error state, it should throw a JException instead.
    enum class Result { Success, FailureTryAgain, FailureFinished };

    // TODO: Deprecate me!
    /// The user is supposed to _throw_ RETURN_STATUS::kNO_MORE_EVENTS or kBUSY from GetEvent()
    enum class RETURN_STATUS { kSUCCESS, kNO_MORE_EVENTS, kBUSY, kTRY_AGAIN, kERROR, kUNKNOWN };


    // Constructor
    // TODO: Deprecate me!
    explicit JEventSource(std::string resource_name, JApplication* app = nullptr)
        : m_resource_name(std::move(resource_name))
        , m_event_count{0}
        {
            m_app = app;
        }

    JEventSource() = default;
    virtual ~JEventSource() = default;


    // `Init` is where the user requests parameters and services. If the user requests all parameters and services here,
    // JANA can report them back to the user without having to open the resource and run the topology.

    virtual void Init() {}

    // To be implemented by the user
    /// `Open` is called by JANA when it is ready to accept events from this event source. The implementor should open
    /// file pointers or sockets here, instead of in the constructor. This is because the implementor won't know how many
    /// or which event sources the user will decide to activate within one job. Thus the implementor can avoid problems
    /// such as running out of file pointers, or multiple event sources attempting to bind to the same socket.

    virtual void Open() {}


    // `Emit` is called by JANA in order to emit a fresh event into the stream, when using CallbackStyle::ExpertMode. 
    // It is very similar to GetEvent(), except the user returns a Result status code instead of throwing an exception.
    // Exceptions are reserved for unrecoverable errors. It accepts an out parameter JEvent. If there is another 
    // entry in the file, or another message waiting at the socket, the user reads the data into the JEvent and returns
    // Result::Success, at which point JANA pushes the JEvent onto the downstream queue. If there is no data waiting yet,
    // the user returns Result::FailureTryAgain, at which point JANA recycles the JEvent to the pool. If there is no more
    // data, the user returns Result::FailureFinished, at which point JANA recycles the JEvent to the pool and calls Close().

    virtual Result Emit(JEvent&) { return Result::Success; };


    /// `Close` is called by JANA when it is finished accepting events from this event source. Here is where you should
    /// cleanly close files, sockets, etc. Although GetEvent() knows when (for instance) there are no more events in a
    /// file, the logic for closing needs to live here because there are other ways a computation may end besides
    /// running out of events in a file. For instance, the user may want to process a limited number of events using
    /// the `jana:nevents` parameter, or they may want to terminate the computation manually using Ctrl-C.

    virtual void Close() {}


    /// `GetEvent` is called by JANA in order to emit a fresh event into the stream. JANA manages the entire lifetime of
    /// the JEvent. The `JEvent` being passed in is empty and merely needs to hydrated as follows:

    ///  1. `SetRunNumber()`
    ///  2. `SetEventNumber()`
    ///  3. `Insert<T>()` raw data of type `T` into the `JEvent` container. Note that `T` should be a child class, such as
    ///     `FADC250DigiHit`, not a parent class such as `JObject` or `TObject`. Also note that `Insert` transfers
    ///     ownership of the data to that JEvent. If the data is shared among multiple `JEvents`, e.g. BOR data,
    ///     use `SetFactoryFlags(JFactory::NOT_OBJECT_OWNER)`

    /// Note that JEvents are usually recycled. Although all reconstruction data is cleared before `GetEvent` is called,
    /// the constituent `JFactories` may retain some state, e.g. statistics, or calibration data keyed off of run number.

    /// If an event cannot be emitted, either because the resource is not ready or because we have reached the end of
    /// the event stream, the implementor should throw the corresponding `RETURN_STATUS`. The user should NEVER throw
    /// `RETURN_STATUS SUCCESS` because this will hurt performance. Instead, they should simply return normally.

    virtual void GetEvent(std::shared_ptr<JEvent>) {};

    virtual void Preprocess(const JEvent&) {};


    /// `FinishEvent` is used to notify the `JEventSource` that an event has been completely processed. This is the final
    /// chance to interact with the `JEvent` before it is either cleared and recycled, or deleted. Although it is
    /// possible to use this for freeing JObjects stored in the JEvent , this is strongly discouraged in favor of putting
    /// that logic on the destructor, RAII-style. Instead, this callback should be used for updating and freeing state
    /// owned by the JEventSource, e.g. raw data which is keyed off of run number and therefore shared among multiple
    /// JEvents. `FinishEvent` is also well-suited for use with `EventGroup`s, e.g. to notify someone that a batch of
    /// events has finished, or to implement "barrier events".
    virtual void FinishEvent(JEvent&) {};


    /// `GetObjects` was historically used for lazily unpacking data from a JEvent and putting it into a "dummy" JFactory.
    /// This mechanism has been replaced by `JEvent::Insert`. All lazy evaluation should happen in a (non-dummy)
    /// JFactory, whereas eager evaluation should happen in `JEventSource::GetEvent` via `JEvent::Insert`.
    virtual bool GetObjects(const std::shared_ptr<const JEvent>&, JFactory*) {
        return false;
    }


    virtual void DoInit() {
        if (m_status == Status::Uninitialized) {
            CallWithJExceptionWrapper("JEventSource::Init", [&](){ Init();});
            m_status = Status::Initialized;
            LOG_INFO(GetLogger()) << "Initialized JEventSource '" << GetTypeName() << "' ('" << GetResourceName() << "')" << LOG_END;
        }
        else {
            throw JException("Attempted to initialize a JEventSource that is not uninitialized!");
        }
    }

    [[deprecated("Replaced by JEventSource::DoOpen()")]]
    virtual void DoInitialize() {
        DoOpen();
    }

    virtual void DoOpen(bool with_lock=true) {
        if (with_lock) {
            std::lock_guard<std::mutex> lock(m_mutex);
            if (m_status != Status::Initialized) {
                throw JException("Attempted to open a JEventSource that hasn't been initialized!");
            }
            CallWithJExceptionWrapper("JEventSource::Open", [&](){ Open();});
            if (GetResourceName().empty()) {
                LOG_INFO(GetLogger()) << "Opened JEventSource '" << GetTypeName() << "'" << LOG_END;
            }
            else {
                LOG_INFO(GetLogger()) << "Opened JEventSource '" << GetTypeName() << "' ('" << GetResourceName() << "')" << LOG_END;
            }
            m_status = Status::Opened;
        }
        else {
            if (m_status != Status::Initialized) {
                throw JException("Attempted to open a JEventSource that hasn't been initialized!");
            }
            CallWithJExceptionWrapper("JEventSource::Open", [&](){ Open();});
            if (GetResourceName().empty()) {
                LOG_INFO(GetLogger()) << "Opened JEventSource '" << GetTypeName() << "'" << LOG_END;
            }
            else {
                LOG_INFO(GetLogger()) << "Opened JEventSource '" << GetTypeName() << "' ('" << GetResourceName() << "')" << LOG_END;
            }
            m_status = Status::Opened;
        }
    }

    virtual void DoClose(bool with_lock=true) {
        if (with_lock) {
            std::lock_guard<std::mutex> lock(m_mutex);

            if (m_status != JEventSource::Status::Opened) return;

            CallWithJExceptionWrapper("JEventSource::Close", [&](){ Close();});
            if (GetResourceName().empty()) {
                LOG_INFO(GetLogger()) << "Closed JEventSource '" << GetTypeName() << "'" << LOG_END;
            }
            else {
                LOG_INFO(GetLogger()) << "Closed JEventSource '" << GetTypeName() << "' ('" << GetResourceName() << "')" << LOG_END;
            }
            m_status = Status::Closed;
        }
        else {
            if (m_status != JEventSource::Status::Opened) return;

            CallWithJExceptionWrapper("JEventSource::Close", [&](){ Close();});
            if (GetResourceName().empty()) {
                LOG_INFO(GetLogger()) << "Closed JEventSource '" << GetTypeName() << "'" << LOG_END;
            }
            else {
                LOG_INFO(GetLogger()) << "Closed JEventSource '" << GetTypeName() << "' ('" << GetResourceName() << "')" << LOG_END;
            }
            m_status = Status::Closed;
        }
    }
    
    Result DoNext(std::shared_ptr<JEvent> event) {

        std::lock_guard<std::mutex> lock(m_mutex); // In general, DoNext must be synchronized.
        
        if (m_status == Status::Uninitialized) {
            throw JException("JEventSource has not been initialized!");
        }

        if (m_callback_style == CallbackStyle::LegacyMode) {
            return DoNextCompatibility(event);
        }

        auto first_evt_nr = m_nskip;
        auto last_evt_nr = m_nevents + m_nskip;

        if (m_status == Status::Initialized) {
            DoOpen(false);
        }
        if (m_status == Status::Opened) {
            if (m_nevents != 0 && (m_event_count == last_evt_nr)) {
                // We exit early (and recycle) because we hit our jana:nevents limit
                DoClose(false);
                return Result::FailureFinished;
            }
            // If we reach this point, we will need to actually read an event

            // We configure the event
            event->SetEventNumber(m_event_count); // Default event number to event count
            event->SetJEventSource(this);
            event->SetSequential(false);
            event->GetJCallGraphRecorder()->Reset();

            // Now we call the new-style interface
            auto previous_origin = event->GetJCallGraphRecorder()->SetInsertDataOrigin( JCallGraphRecorder::ORIGIN_FROM_SOURCE);  // (see note at top of JCallGraphRecorder.h)
            JEventSource::Result result;
            CallWithJExceptionWrapper("JEventSource::Emit", [&](){
                result = Emit(*event);
            });
            event->GetJCallGraphRecorder()->SetInsertDataOrigin( previous_origin );

            if (result == Result::Success) {
                m_event_count += 1; 
                // We end up here if we read an entry in our file or retrieved a message from our socket,
                // and believe we could obtain another one immediately if we wanted to
                for (auto* output : m_outputs) {
                    output->InsertCollection(*event);
                }
                if (m_event_count <= first_evt_nr) {
                    // We immediately throw away this whole event because of nskip 
                    // (although really we should be handling this with Seek())
                    return Result::FailureTryAgain;
                }
                return Result::Success;
            }
            else if (result == Result::FailureFinished) {
                // We end up here if we tried to read an entry in a file, but found EOF
                // or if we received a message from a socket that contained no data and indicated no more data will be coming
                DoClose(false);
                return Result::FailureFinished;
            }
            else if (result == Result::FailureTryAgain) {
                // We end up here if we tried to read an entry in a file but it is on a tape drive and isn't ready yet
                // or if we polled the socket, found no new messages, but still expect messages later
                return Result::FailureTryAgain;
            }
            else {
                throw JException("Invalid JEventSource::Result value!");
            }
        }
        else { // status == Closed
            return Result::FailureFinished;
        }
    }

    Result DoNextCompatibility(std::shared_ptr<JEvent> event) {

        auto first_evt_nr = m_nskip;
        auto last_evt_nr = m_nevents + m_nskip;

        try {
            if (m_status == Status::Initialized) {
                DoOpen(false);
            }
            if (m_status == Status::Opened) {
                if (m_event_count < first_evt_nr) {
                    // Skip these events due to nskip
                    event->SetEventNumber(m_event_count); // Default event number to event count
                    auto previous_origin = event->GetJCallGraphRecorder()->SetInsertDataOrigin( JCallGraphRecorder::ORIGIN_FROM_SOURCE);  // (see note at top of JCallGraphRecorder.h)
                    GetEvent(event);
                    event->GetJCallGraphRecorder()->SetInsertDataOrigin( previous_origin );
                    m_event_count += 1;
                    return Result::FailureTryAgain;  // Reject this event and recycle it
                } else if (m_nevents != 0 && (m_event_count == last_evt_nr)) {
                    // Declare ourselves finished due to nevents
                    DoClose(false); // Close out the event source as soon as it declares itself finished
                    return Result::FailureFinished;
                } else {
                    // Actually emit an event.
                    // GetEvent() expects the following things from its incoming JEvent
                    event->SetEventNumber(m_event_count);
                    event->SetJApplication(m_app);
                    event->SetJEventSource(this);
                    event->SetSequential(false);
                    event->GetJCallGraphRecorder()->Reset();
                    auto previous_origin = event->GetJCallGraphRecorder()->SetInsertDataOrigin( JCallGraphRecorder::ORIGIN_FROM_SOURCE);  // (see note at top of JCallGraphRecorder.h)
                    GetEvent(event);
                    for (auto* output : m_outputs) {
                        output->InsertCollection(*event);
                    }
                    event->GetJCallGraphRecorder()->SetInsertDataOrigin( previous_origin );
                    m_event_count += 1;
                    return Result::Success; // Don't reject this event!
                }
            } else if (m_status == Status::Closed) {
                return Result::FailureFinished;
            } else {
                throw JException("Invalid m_status");
            }
        }
        catch (RETURN_STATUS rs) {

            if (rs == RETURN_STATUS::kNO_MORE_EVENTS) {
                DoClose(false);
                return Result::FailureFinished;
            }
            else if (rs == RETURN_STATUS::kTRY_AGAIN || rs == RETURN_STATUS::kBUSY) {
                return Result::FailureTryAgain;
            }
            else if (rs == RETURN_STATUS::kERROR || rs == RETURN_STATUS::kUNKNOWN) {
                JException ex ("JEventSource threw RETURN_STATUS::kERROR or kUNKNOWN");
                ex.plugin_name = m_plugin_name;
                ex.type_name = m_type_name;
                ex.function_name = "JEventSource::GetEvent";
                ex.instance_name = m_resource_name;
                throw ex;
            }
            else {
                return Result::Success;
            }
        }
        catch (JException& ex) {
            if (ex.function_name.empty()) ex.function_name = "JEventSource::GetEvent";
            if (ex.type_name.empty()) ex.type_name = m_type_name;
            if (ex.instance_name.empty()) ex.instance_name = m_prefix;
            if (ex.plugin_name.empty()) ex.plugin_name = m_plugin_name;
            throw ex;
        }
        catch (std::exception& e){
            auto ex = JException(e.what());
            ex.exception_type = JTypeInfo::demangle_current_exception_type();
            ex.nested_exception = std::current_exception();
            ex.function_name = "JEventSource::GetEvent";
            ex.type_name = m_type_name;
            ex.instance_name = m_prefix;
            ex.plugin_name = m_plugin_name;
            throw ex;
        }
        catch (...) {
            auto ex = JException("Unknown exception");
            ex.exception_type = JTypeInfo::demangle_current_exception_type();
            ex.nested_exception = std::current_exception();
            ex.function_name = "JEventSource::GetEvent";
            ex.type_name = m_type_name;
            ex.instance_name = m_prefix;
            ex.plugin_name = m_plugin_name;
            throw ex;
        }
    }

    /// Calls the optional-and-discouraged user-provided FinishEvent virtual method, enforcing
    /// 1. Thread safety
    /// 2. The m_enable_free_event flag

    void DoFinish(JEvent& event) {
        if (m_enable_free_event) {
            std::lock_guard<std::mutex> lock(m_mutex);
            CallWithJExceptionWrapper("JEventSource::FinishEvent", [&](){
                FinishEvent(event);
            });
        }
    }

    void Summarize(JComponentSummary& summary) {

        auto* result = new JComponentSummary::Component(
                JComponentSummary::ComponentType::Source, GetPrefix(), GetTypeName(), GetLevel(), GetPluginName());

        for (const auto* output : m_outputs) {
            size_t suboutput_count = output->collection_names.size();
            for (size_t i=0; i<suboutput_count; ++i) {
                result->AddOutput(new JComponentSummary::Collection("", output->collection_names[i], output->type_name, GetLevel()));
            }
        }

        summary.Add(result);
    }

    // Getters and setters
    
    void SetResourceName(std::string resource_name) { m_resource_name = resource_name; }

    std::string GetResourceName() const { return m_resource_name; }

    uint64_t GetEventCount() const { return m_event_count; };

    // TODO: Deprecate me
    virtual std::string GetType() const { return m_type_name; }

    // TODO: Deprecate me
    std::string GetName() const { return m_resource_name; }

    // TODO: Deprecate me
    virtual std::string GetVDescription() const {
        return "<description unavailable>";
    } ///< Optional for getting description via source rather than JEventSourceGenerator


    uint64_t GetNSkip() { return m_nskip; }
    uint64_t GetNEvents() { return m_nevents; }

    // Meant to be called by user
    /// EnableFinishEvent() is intended to be called by the user in the constructor in order to
    /// tell JANA to call the provided FinishEvent method after all JEventProcessors
    /// have finished with a given event. This should only be enabled when absolutely necessary
    /// (e.g. for backwards compatibility) because it introduces contention for the JEventSource mutex,
    /// which will hurt performance. Conceptually, FinishEvent isn't great, and so should be avoided when possible.
    void EnableFinishEvent() { m_enable_free_event = true; }

    // Meant to be called by JANA
    void SetNEvents(uint64_t nevents) { m_nevents = nevents; };

    // Meant to be called by JANA
    void SetNSkip(uint64_t nskip) { m_nskip = nskip; };


private:
    std::string m_resource_name;
    std::atomic_ullong m_event_count {0};
    uint64_t m_nskip = 0;
    uint64_t m_nevents = 0;
    bool m_enable_free_event = false;

};


