
// Copyright 2020, Jefferson Science Associates, LLC.
// Subject to the terms in the LICENSE file found in the top-level directory.

#pragma once

#include <JANA/Components/JComponent.h>
#include <JANA/Components/JHasOutputs.h>
#include <JANA/JEvent.h>
#include <JANA/JException.h>
#include <JANA/JFactoryGenerator.h>


class JFactoryGenerator;
class JApplication;
class JFactory;


class JEventSource : public jana::components::JComponent,
                     public jana::components::JHasOutputs {

public:
    /// Result describes what happened the last time a GetEvent() was attempted.
    /// If Emit() or GetEvent() reaches an error state, it should throw a JException instead.
    enum class Result { Success, FailureTryAgain, FailureFinished, FailureLevelChange };

    /// The user is supposed to _throw_ RETURN_STATUS::kNO_MORE_EVENTS or kBUSY from GetEvent()
    enum class RETURN_STATUS { kSUCCESS, kNO_MORE_EVENTS, kBUSY, kTRY_AGAIN, kERROR, kUNKNOWN };


private:
    std::string m_resource_name;
    std::atomic_ullong m_events_emitted {0};
    std::atomic_ullong m_events_skipped {0};
    std::atomic_ullong m_events_processed {0};
    uint64_t m_nskip = 0;
    uint64_t m_nevents = 0;
    bool m_enable_finish_event = false;
    bool m_enable_get_objects = false;
    bool m_enable_process_parallel = false;

    std::vector<JEventLevel> m_event_levels;
    JEventLevel m_next_level = JEventLevel::None;


public:
    explicit JEventSource(std::string resource_name, JApplication* app = nullptr)
        : m_resource_name(std::move(resource_name)) {
            m_app = app;
        }

    JEventSource() {
        m_type_name = "JEventSource";
    }
    virtual ~JEventSource() = default;



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


    /// For work that should be done in parallel on a JEvent, but is tightly coupled to the JEventSource for some reason.
    /// Called after Emit() by JEventMapArrow, but only if EnableProcessParallel(true) is set. Note that the JEvent& is not
    /// const here, because we need to be able to call event.Insert() from here. Also note that `this` IS const, because
    /// it is not safe to access any state in parallel from here. Note that this includes things like calibration constants.
    /// If you need to safely access state, put use a JFactory instead.
    virtual void ProcessParallel(JEvent&) const {};


    /// `FinishEvent` is used to notify the `JEventSource` that an event has been completely processed. This is the final
    /// chance to interact with the `JEvent` before it is either cleared and recycled, or deleted. Although it is
    /// possible to use this for freeing JObjects stored in the JEvent , this is strongly discouraged in favor of putting
    /// that logic on the destructor, RAII-style. Instead, this callback should be used for updating and freeing state
    /// owned by the JEventSource, e.g. raw data which is keyed off of run number and therefore shared among multiple
    /// JEvents. `FinishEvent` is also well-suited for use with `EventGroup`s, e.g. to notify someone that a batch of
    /// events has finished, or to implement "barrier events".

    virtual void FinishEvent(JEvent&) {};


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


    /// `GetObjects` was historically used for lazily unpacking data from a JEvent and putting it into a "dummy" JFactory.
    /// This mechanism has been replaced by `JEvent::Insert`. All lazy evaluation should happen in a (non-dummy)
    /// JFactory, whereas eager evaluation should happen in `JEventSource::GetEvent` via `JEvent::Insert`.

    virtual bool GetObjects(const std::shared_ptr<const JEvent>&, JFactory*) {
        return false;
    }

    /// `Skip` allows the user to move forward in the file without having to read and discard entire events. It takes
    /// as inputs an event object and the number of events to skip, and returns a pair containing a JEventSource::Result
    /// and the number of events that still need to be skipped. The event object can be completely ignored. If it is
    /// populated, however, it should be cleared before Skip() returns.
    virtual std::pair<JEventSource::Result, size_t> Skip(JEvent& event, size_t events_to_skip);


    // Getters
    
    std::string GetResourceName() const { return m_resource_name; }

    uint64_t GetEmittedEventCount() const { return m_events_emitted; };
    uint64_t GetSkippedEventCount() const { return m_events_skipped; };
    uint64_t GetProcessedEventCount() const { return m_events_processed; };

    const std::vector<JEventLevel> GetEventLevels() { return m_event_levels; }

    bool IsGetObjectsEnabled() const { return m_enable_get_objects; }
    bool IsFinishEventEnabled() const { return m_enable_finish_event; }
    bool IsProcessParallelEnabled() const { return m_enable_process_parallel; }

    uint64_t GetNSkip() { return m_nskip; }
    uint64_t GetNEvents() { return m_nevents; }

    virtual std::string GetVDescription() const {
        return "<description unavailable>";
    } ///< Optional for getting description via source rather than JEventSourceGenerator


    // Setters

    void SetResourceName(std::string resource_name) { m_resource_name = resource_name; }

    /// EnableFinishEvent() is intended to be called by the user in the constructor in order to
    /// tell JANA to call the provided FinishEvent method after all JEventProcessors
    /// have finished with a given event. This should only be enabled when absolutely necessary
    /// (e.g. for backwards compatibility) because it introduces contention for the JEventSource mutex,
    /// which will hurt performance. Conceptually, FinishEvent isn't great, and so should be avoided when possible.
    void EnableFinishEvent(bool enable=true) { m_enable_finish_event = enable; }
    void EnableGetObjects(bool enable=true) { m_enable_get_objects = enable; }
    void EnableProcessParallel(bool enable=true) { m_enable_process_parallel = enable; }

    void SetNEvents(uint64_t nevents) { m_nevents = nevents; };
    void SetNSkip(uint64_t nskip) { m_nskip = nskip; };

    void SetNextEventLevel(JEventLevel level) { m_next_level = level; }
    void SetEventLevels(std::vector<JEventLevel> levels) { m_event_levels = levels; }
    JEventLevel GetNextInputLevel() const { return m_next_level; }


    // Internal

    void DoOpen(bool with_lock=true);

    void DoClose(bool with_lock=true);

    Result DoNext(std::shared_ptr<JEvent> event);

    Result DoNextCompatibility(std::shared_ptr<JEvent> event);

    void DoFinishEvent(JEvent& event);

    void Summarize(JComponentSummary& summary) const override;


};


