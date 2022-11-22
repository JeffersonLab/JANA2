
// Copyright 2020, Jefferson Science Associates, LLC.
// Subject to the terms in the LICENSE file found in the top-level directory.


#ifndef _JEventSource_h_
#define _JEventSource_h_

#include <JANA/JException.h>
#include <JANA/Utils/JTypeInfo.h>
#include <JANA/JEvent.h>
#include <JANA/JFactoryGenerator.h>

#include <string>
#include <atomic>
#include <memory>
#include <mutex>

class JFactoryGenerator;
class JApplication;
class JFactory;


class JEventSource {

public:

    /// SourceStatus describes the current state of the EventSource
    enum class SourceStatus { Unopened, Opened, Finished };

    /// ReturnStatus describes what happened the last time a GetEvent() was attempted.
    /// If GetEvent() reaches an error state, it should throw a JException instead.
    enum class ReturnStatus { Success, TryAgain, Finished };

    /// The user is supposed to _throw_ RETURN_STATUS::kNO_MORE_EVENTS or kBUSY from GetEvent()
    enum class RETURN_STATUS { kSUCCESS, kNO_MORE_EVENTS, kBUSY, kTRY_AGAIN, kERROR, kUNKNOWN };


    // Constructor

    explicit JEventSource(std::string resource_name, JApplication* app = nullptr)
        : m_resource_name(std::move(resource_name))
        , m_application(app)
        , m_factory_generator(nullptr)
        , m_status(SourceStatus::Unopened)
        , m_event_count{0}
        {}

    virtual ~JEventSource() = default;



    // To be implemented by the user
    /// `Open` is called by JANA when it is ready to accept events from this event source. The implementor should open
    /// file pointers or sockets here, instead of in the constructor. This is because the implementor won't know how many
    /// or which event sources the user will decide to activate within one job. Thus the implementor can avoid problems
    /// such as running out of file pointers, or multiple event sources attempting to bind to the same socket.

    virtual void Open() {}


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

    virtual void GetEvent(std::shared_ptr<JEvent>) = 0;


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


    // Wrappers for calling Open and GetEvent in a safe way

    virtual void DoInitialize() {
        try {
            std::call_once(m_init_flag, &JEventSource::Open, this);
            m_status = SourceStatus::Opened;
        }
        catch (JException& ex) {
            ex.plugin_name = m_plugin_name;
            ex.component_name = GetType();
            throw ex;
        }
        catch (std::runtime_error& e){
            throw(JException(e.what()));
        }
        catch (...) {
            auto ex = JException("Unknown exception in JEventSource::Open()");
            ex.nested_exception = std::current_exception();
            ex.plugin_name = m_plugin_name;
            ex.component_name = GetType();
            throw ex;
        }
    }

    virtual void DoFinalize() {
        try {
            std::call_once(m_close_flag, &JEventSource::Close, this);
            m_status = SourceStatus::Finished;
        }
        catch (JException& ex) {
            ex.plugin_name = m_plugin_name;
            ex.component_name = GetType();
            throw ex;
        }
        catch (std::runtime_error& e){
            throw(JException(e.what()));
        }
        catch (...) {
            auto ex = JException("Unknown exception in JEventSource::Close()");
            ex.nested_exception = std::current_exception();
            ex.plugin_name = m_plugin_name;
            ex.component_name = GetType();
            throw ex;
        }
    }

    ReturnStatus DoNext(std::shared_ptr<JEvent> event) {

        std::lock_guard<std::mutex> lock(m_mutex); // In general, DoNext must be synchronized.
        auto first_evt_nr = m_nskip;
        auto last_evt_nr = m_nevents + m_nskip;

        try {
            if (m_status == SourceStatus::Unopened) {
                DoInitialize();
                m_status = SourceStatus::Opened;
            }
            if (m_status == SourceStatus::Opened) {
                if (m_event_count < first_evt_nr) {
                    // Skip these events due to nskip
                    event->SetEventNumber(m_event_count); // Default event number to event count
                    auto previous_origin = event->GetJCallGraphRecorder()->SetInsertDataOrigin( JCallGraphRecorder::ORIGIN_FROM_SOURCE);  // (see note at top of JCallGraphRecorder.h)
                    GetEvent(event);
                    event->GetJCallGraphRecorder()->SetInsertDataOrigin( previous_origin );
                    m_event_count += 1;
                    return ReturnStatus::TryAgain;  // Reject this event and recycle it
                } else if (m_nevents != 0 && (m_event_count == last_evt_nr)) {
                    // Declare ourselves finished due to nevents
                    DoFinalize(); // Close out the event source as soon as it declares itself finished
                    return ReturnStatus::Finished;
                } else {
                    // Actually emit an event.
                    // GetEvent() expects the following things from its incoming JEvent
                    event->SetEventNumber(m_event_count);
                    event->SetJApplication(m_application);
                    event->SetJEventSource(this);
                    event->SetSequential(false);
                    event->GetJCallGraphRecorder()->Reset();
                    if (event->GetJEventSource() != this && m_factory_generator != nullptr) {
                        // If we have multiple event sources, we need to make sure we are using
                        // event-source-specific factories on top of the default ones.
                        auto factory_set = new JFactorySet();
                        m_factory_generator->GenerateFactories(factory_set);
                        factory_set->Merge(*event->GetFactorySet());
                        event->SetFactorySet(factory_set);
                    }
                    auto previous_origin = event->GetJCallGraphRecorder()->SetInsertDataOrigin( JCallGraphRecorder::ORIGIN_FROM_SOURCE);  // (see note at top of JCallGraphRecorder.h)
                    GetEvent(event);
                    event->GetJCallGraphRecorder()->SetInsertDataOrigin( previous_origin );
                    m_event_count += 1;
                    return ReturnStatus::Success; // Don't reject this event!
                }
            } else if (m_status == SourceStatus::Finished) {
                return ReturnStatus::Finished;
            } else {
                throw JException("Invalid ReturnStatus");
            }
        }
        catch (RETURN_STATUS rs) {

            if (rs == RETURN_STATUS::kNO_MORE_EVENTS) {
                DoFinalize();
                return ReturnStatus::Finished;
            }
            else if (rs == RETURN_STATUS::kTRY_AGAIN || rs == RETURN_STATUS::kBUSY) {
                return ReturnStatus::TryAgain;
            }
            else if (rs == RETURN_STATUS::kERROR || rs == RETURN_STATUS::kUNKNOWN) {
                JException ex ("JEventSource threw RETURN_STATUS::kERROR or kUNKNOWN");
                ex.plugin_name = m_plugin_name;
                ex.component_name = GetType();
                throw ex;
            }
            else {
                return ReturnStatus::Success;
            }
        }
        catch (JException& ex) {
            ex.plugin_name = m_plugin_name;
            ex.component_name = GetType();
            throw ex;
        }
        catch (std::runtime_error& e){
            throw(JException(e.what()));
        }
        catch (...) {
            auto ex = JException("Unknown exception in JEventSource::GetEvent()");
            ex.nested_exception = std::current_exception();
            ex.plugin_name = m_plugin_name;
            ex.component_name = GetType();
            throw ex;
        }
    }

    /// Calls the optional-and-discouraged user-provided FinishEvent virtual method, enforcing
    /// 1. Thread safety
    /// 2. The m_enable_free_event flag

    void DoFinish(JEvent& event) {
        if (m_enable_free_event) {
            std::lock_guard<std::mutex> lock(m_mutex);
            FinishEvent(event);
        }
    }


    // Getters and setters

    SourceStatus GetStatus() const { return m_status; }

    std::string GetPluginName() const { return m_plugin_name; }

    std::string GetTypeName() const { return m_type_name; }

    std::string GetResourceName() const { return m_resource_name; }

    uint64_t GetEventCount() const { return m_event_count; };

    JApplication* GetApplication() const { return m_application; }

    virtual std::string GetType() const { return m_type_name; }

    std::string GetName() const { return m_resource_name; }

    virtual std::string GetVDescription() const {
        return "<description unavailable>";
    } ///< Optional for getting description via source rather than JEventSourceGenerator

    //This should create default factories for all types available in the event source
    JFactoryGenerator* GetFactoryGenerator() const { return m_factory_generator; }

    uint64_t GetNSkip() { return m_nskip; }
    uint64_t GetNEvents() { return m_nevents; }


    /// SetTypeName is intended as a replacement to GetType(), which should be less confusing for the
    /// user. It should be called from the constructor. For convenience, we provide a
    /// NAME_OF_THIS macro so that the user doesn't have to type the class name as a string, which may
    /// get out of sync if automatic refactoring tools are used.

    // Meant to be called by user
    void SetTypeName(std::string type_name) { m_type_name = std::move(type_name); }

    // Meant to be called by user
    /// SetFactoryGenerator allows us to override the set of factories. This is somewhat superfluous
    /// The only time we _really_ need to call SetFactoryGenerator
    void SetFactoryGenerator(JFactoryGenerator* generator) { m_factory_generator = generator; }

    // Meant to be called by user
    /// EnableFinishEvent() is intended to be called by the user in the constructor in order to
    /// tell JANA to call the provided FinishEvent method after all JEventProcessors
    /// have finished with a given event. This should only be enabled when absolutely necessary
    /// (e.g. for backwards compatibility) because it introduces contention for the JEventSource mutex,
    /// which will hurt performance. Conceptually, FinishEvent isn't great, and so should be avoided when possible.
    void EnableFinishEvent() { m_enable_free_event = true; }

    // Meant to be called by JANA
    void SetApplication(JApplication* app) { m_application = app; }

    // Meant to be called by JANA
    void SetPluginName(std::string plugin_name) { m_plugin_name = std::move(plugin_name); };

    // Meant to be called by JANA
    void SetNEvents(uint64_t nevents) { m_nevents = nevents; };

    // Meant to be called by JANA
    void SetNSkip(uint64_t nskip) { m_nskip = nskip; };


private:
    std::string m_resource_name;
    JApplication* m_application = nullptr;
    JFactoryGenerator* m_factory_generator = nullptr;
    std::atomic<SourceStatus> m_status;
    std::atomic_ullong m_event_count {0};
    uint64_t m_nskip = 0;
    uint64_t m_nevents = 0;
    std::string m_plugin_name;
    std::string m_type_name;
    std::once_flag m_init_flag;
    std::once_flag m_close_flag;
    std::mutex m_mutex;
    bool m_enable_free_event = false;
};

#endif // _JEventSource_h_

