
// Copyright 2023, Jefferson Science Associates, LLC.
// Subject to the terms in the LICENSE file found in the top-level directory.

#include <JANA/JFactory.h>
#include <JANA/JEvent.h>
#include <JANA/JEventSource.h>
#include <JANA/Utils/JTypeInfo.h>
#include <JANA/JVersion.h>

#if JANA2_HAVE_PERFETTO
#include <JANA/Services/JPerfettoService.h>

// Thread-local pointer to the factory whose span is currently open on this thread.
// Used to build inter-factory dependency flow arrows: when factory B is triggered from
// within factory A's Process(), g_current_executing_factory == A, so we can draw
// an arrow A → B in the Perfetto timeline.
static thread_local JFactory* g_current_executing_factory = nullptr;
#endif


class FlagGuard {
    bool* m_flag;
public:
    FlagGuard(bool* flag) : m_flag(flag) {
        *m_flag = true;
    }
    ~FlagGuard() {
        *m_flag = false;
    }
};

void JFactory::Create(const std::shared_ptr<const JEvent>& event) {
    Create(*event.get());
}

void JFactory::Create(const JEvent& event) {

    if (mInsideCreate && (mStatus != Status::Inserted)) {
        // Ideally, we disallow any calls to Create() that end up calling it right back. However, we do allow
        // calls that go down to GetObjects, who inserts the data, but then RETRIEVES the same data it just inserted,
        // so that it can subsequently calculate and insert OTHER data. Once we refactor JEventSourceEVIOpp, we can consider
        // removing this weird edge case.
        throw JException("Encountered a cycle in the factory dependency graph! Hint: Maybe this data was supposed to be inserted in the JEventSource");
    }
    FlagGuard insideCreateFlagGuard (&mInsideCreate); // No matter how we exit from Create() (particularly with exceptions) mInsideCreate will be set back to false

    if (m_app == nullptr && event.GetJApplication() != nullptr) {
        // These are usually set by JFactoryGeneratorT, but some user code has custom JFactoryGenerators which don't!
        // The design of JFactoryGenerator doesn't give us a better place to inject things
        m_app = event.GetJApplication();
        m_logger = m_app->GetJParameterManager()->GetLogger(GetLoggerName());
    }

    // How do we obtain our data? The priority is as follows:
    // 1. JFactory::Process() if REGENERATE flag is set
    // 2. JEvent::Insert()
    // 3. JEventSource::GetObjects() if source has GetObjects() enabled
    // 4. JFactory::Process()

    // ---------------------------------------------------------------------
    // 1. JFactory::Process() if REGENERATE flag is set
    // ---------------------------------------------------------------------

    if (mRegenerate) {
        if (mStatus == Status::Inserted) {
            // Status::Inserted indicates that the data came from either src->GetObjects() or evt->Insert()
            ClearData(); 
            // ClearData() resets mStatus to Unprocessed so that the data will be regenerated exactly once.
        }
        // After this point, control flow falls through to "4. JFactory::Process"
    }
    else {

        // ---------------------------------------------------------------------
        // 2. JEvent::Insert()
        // ---------------------------------------------------------------------

        if (mStatus == Status::Inserted) {
            // This may include data cached from eventsource->GetObjects().
            // Either way, short-circuit here, because the data is present.
            return;
        }

        // ---------------------------------------------------------------------
        // 3. JEventSource::GetObjects() if source has GetObjects() enabled
        // ---------------------------------------------------------------------

        auto src = event.GetJEventSource();
        if (src != nullptr && src->IsGetObjectsEnabled()) {
            bool found_data = false;

            CallWithJExceptionWrapper("JEventSource::GetObjects", [&](){ 
                found_data = src->GetObjects(event.shared_from_this(), this); });

            if (found_data) {
                mStatus = Status::Inserted;
                return;
            }
        }
        // If neither "2. JEvent::Insert()" nor "3. JEventSource::GetObjects()" succeeded, fall through to "4. JFactory::Process()"
    }

    // ---------------------------------------------------------------------
    // 4. JFactory::Process()
    // ---------------------------------------------------------------------

    // Check if init had _previously_ excepted but the cache was since cleared
    if (mInitStatus == InitStatus::InitExcepted && mStatus == Status::Empty) {
        for (auto* output : GetOutputs()) {
            output->LagrangianStore(*event.GetFactorySet(), JDatabundle::Status::Excepted);
        }
        for (auto* output : GetVariadicOutputs()) {
            output->LagrangianStore(*event.GetFactorySet(), JDatabundle::Status::Excepted);
        }
        mStatus = Status::Excepted;
        std::rethrow_exception(mException);
    }

    // The factory span wraps the entire activation: Init + run callbacks + input population + Process.
    // factory_init spans (Init, BeginRun, ChangeRun, EndRun) appear as children of this factory span,
    // making the phase structure visible when inspecting any factory span in the trace.
    // Flow events connect each factory span to the parent factory that triggered it,
    // drawn as arrows showing the data-dependency chain in the Perfetto UI.
    {
#if JANA2_HAVE_PERFETTO
        // If a parent factory is currently executing on this thread it triggered us.
        // Emit a flow-start instant inside the parent's still-open span, then open
        // our span with a TerminatingFlow — Perfetto draws an arrow parent → us.
        JFactory* const caller = g_current_executing_factory;
        if (caller != nullptr) {
            // FNV-1a-inspired hash: unique 64-bit flow ID per (caller, callee, event).
            uint64_t flow_id = 14695981039346656037ULL;
            flow_id = (flow_id ^ reinterpret_cast<uint64_t>(caller)) * 1099511628211ULL;
            flow_id = (flow_id ^ reinterpret_cast<uint64_t>(this))   * 1099511628211ULL;
            flow_id = (flow_id ^ static_cast<uint64_t>(event.GetEventNumber())) * 1099511628211ULL;
            // Flow starts here — we are still executing inside the parent's open span.
            TRACE_EVENT_INSTANT("factory", perfetto::DynamicString{GetFactoryName()},
                perfetto::ThreadTrack::Current(), perfetto::Flow::Global(flow_id));
            // Our span starts, consuming the arrow from the parent's instant above.
            TRACE_EVENT_BEGIN("factory", perfetto::DynamicString{GetFactoryName()},
                perfetto::TerminatingFlow::Global(flow_id),
                "tag", GetTag(), "event_nr", event.GetEventNumber());
        } else {
            TRACE_EVENT_BEGIN("factory", perfetto::DynamicString{GetFactoryName()},
                "tag", GetTag(), "event_nr", event.GetEventNumber());
        }
        // RAII: close the Perfetto span on scope exit (exception-safe).
        struct SpanGuard { ~SpanGuard() { TRACE_EVENT_END("factory"); } } span_guard;
        // RAII: track the currently-executing factory so sub-factories can link back to us.
        // Declared after span_guard — its destructor runs first, restoring the caller
        // before the span closes.
        struct CallerGuard {
            JFactory** slot; JFactory* saved;
            ~CallerGuard() { *slot = saved; }
        } caller_guard{&g_current_executing_factory, caller};
        g_current_executing_factory = this;
#endif

        // Make sure Init() ran, which might except...
        try {
            DoInit(); // This checks mInitStatus internally before calling Init()
        }
        catch(...) {
            // If Init() excepts, we still need to store an empty collection
            mStatus = Status::Excepted;
            for (auto* output : GetOutputs()) {
                output->LagrangianStore(*event.GetFactorySet(), JDatabundle::Status::Excepted);
            }
            for (auto* output : GetVariadicOutputs()) {
                output->LagrangianStore(*event.GetFactorySet(), JDatabundle::Status::Excepted);
            }
            std::rethrow_exception(mException);
        }

        // At this point, Init() has run and has _not_ excepted

        if (mStatus == Status::Excepted) {
            // But Process() might have already excepted!
            std::rethrow_exception(mException);
        }
        else if (mStatus == Status::Empty) {
            // Now we know that we need to run Process() to create the data in the first place
            try {
                auto run_number = event.GetRunNumber();
                if (mPreviousRunNumber != run_number) {
                    if (m_callback_style == CallbackStyle::LegacyMode) {
                        if (mPreviousRunNumber != -1) {
                            {
#if JANA2_HAVE_PERFETTO
                                TRACE_EVENT("factory_init", perfetto::DynamicString{GetFactoryName()},
                                    "tag", GetTag(), "run_nr", run_number, "phase", "EndRun");
#endif
                                CallWithJExceptionWrapper("JFactory::EndRun", [&](){ EndRun(); });
                            }
                        }
                        {
#if JANA2_HAVE_PERFETTO
                            TRACE_EVENT("factory_init", perfetto::DynamicString{GetFactoryName()},
                                "tag", GetTag(), "run_nr", run_number, "phase", "ChangeRun");
#endif
                            CallWithJExceptionWrapper("JFactory::ChangeRun", [&](){ ChangeRun(event.shared_from_this()); });
                        }
                        {
#if JANA2_HAVE_PERFETTO
                            TRACE_EVENT("factory_init", perfetto::DynamicString{GetFactoryName()},
                                "tag", GetTag(), "run_nr", run_number, "phase", "BeginRun");
#endif
                            CallWithJExceptionWrapper("JFactory::BeginRun", [&](){ BeginRun(event.shared_from_this()); });
                        }
                    }
                    else if (m_callback_style == CallbackStyle::ExpertMode) {
                        {
#if JANA2_HAVE_PERFETTO
                            TRACE_EVENT("factory_init", perfetto::DynamicString{GetFactoryName()},
                                "tag", GetTag(), "run_nr", run_number, "phase", "ChangeRun");
#endif
                            CallWithJExceptionWrapper("JFactory::ChangeRun", [&](){ ChangeRun(event); });
                        }
                    }
                    mPreviousRunNumber = run_number;
                }
                for (auto* input : GetInputs()) {
                    input->Populate(event);
                }
                for (auto* input : GetVariadicInputs()) {
                    input->Populate(event);
                }
                // Process() runs inside the factory span; dependent factory spans appear as children
                if (m_callback_style == CallbackStyle::LegacyMode) {
                    CallWithJExceptionWrapper("JFactory::Process", [&](){ Process(event.shared_from_this()); });
                }
                else if (m_callback_style == CallbackStyle::ExpertMode) {
                    CallWithJExceptionWrapper("JFactory::Process", [&](){ Process(event); });
                }
                else {
                    throw JException("Invalid callback style");
                }
            }
            catch (...) {
                // Save everything already created even if we throw an exception
                // This is so that we leave everything in a valid state just in case someone tries to catch the exception recover,
                // such as EICrecon. (Remember that a missing collection in the podio frame will segfault if anyone tries to write that frame)
                // Note that the collections themselves won't know that they exited early

                LOG << "Exception in JFactory::Create, prefix=" << GetPrefix();
                mStatus = Status::Excepted;
                mException = std::current_exception();
                for (auto* output : GetOutputs()) {
                    output->LagrangianStore(*event.GetFactorySet(), JDatabundle::Status::Excepted);
                }
                for (auto* output : GetVariadicOutputs()) {
                    output->LagrangianStore(*event.GetFactorySet(), JDatabundle::Status::Excepted);
                }
                throw;
            }

            // Save the (successfully processed) data
            mStatus = Status::Processed;
            for (auto* output : GetOutputs()) {
                output->LagrangianStore(*event.GetFactorySet(), JDatabundle::Status::Created);
            }
            for (auto* output : GetVariadicOutputs()) {
                output->LagrangianStore(*event.GetFactorySet(), JDatabundle::Status::Created);
            }
        }
    } // end factory span
}

void JFactory::DoInit() {
    if (mInitStatus != InitStatus::InitNotRun) {
        return;
    }
    for (auto* parameter : m_parameters) {
        parameter->Init(*(m_app->GetJParameterManager()), m_prefix);
    }
    for (auto* service : m_services) {
        service->Fetch(m_app);
    }
    try {
        {
#if JANA2_HAVE_PERFETTO
            TRACE_EVENT("factory_init", perfetto::DynamicString{GetFactoryName()},
                "tag", GetTag(), "phase", "Init");
#endif
            CallWithJExceptionWrapper("JFactory::Init", [&](){ Init(); });
        }
        mInitStatus = InitStatus::InitRun;
    }
    catch (...) {
        mInitStatus = InitStatus::InitExcepted;
        mException = std::current_exception();
        throw;
    }
}

void JFactory::DoFinish() {
    if (mInitStatus == InitStatus::InitRun) {
        if (mPreviousRunNumber != -1) {
            CallWithJExceptionWrapper("JFactory::EndRun", [&](){ EndRun(); });
        }
        CallWithJExceptionWrapper("JFactory::Finish", [&](){ Finish(); });
    }
}

void JFactory::Summarize(JComponentSummary& summary) const {

    auto fs = new JComponentSummary::Component(
            "Factory",
            GetPrefix(),
            GetTypeName(),
            GetLevel(),
            GetPluginName());

    SummarizeInputs(*fs);
    SummarizeOutputs(*fs);
    summary.Add(fs);
}


