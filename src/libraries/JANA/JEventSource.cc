#include <JANA/JEventSource.h>

void JEventSource::DoInit() {
    std::lock_guard<std::mutex> lock(m_mutex);
    if (m_status != Status::Uninitialized) {
        throw JException("Attempted to initialize a JEventSource that is already initialized!");
    }
    for (auto* parameter : m_parameters) {
        parameter->Configure(*(m_app->GetJParameterManager()), m_prefix);
    }
    for (auto* service : m_services) {
        service->Fetch(m_app);
    }
    CallWithJExceptionWrapper("JEventSource::Init", [&](){ Init(); });
    m_status = Status::Initialized;
    LOG_INFO(GetLogger()) << "Initialized JEventSource '" << GetTypeName() << "' ('" << GetResourceName() << "')" << LOG_END;
}

void JEventSource::DoInitialize() {
    DoOpen();
}

void JEventSource::DoOpen(bool with_lock) {
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

void JEventSource::DoClose(bool with_lock) {
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

JEventSource::Result JEventSource::DoNext(std::shared_ptr<JEvent> event) {

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
        if (m_nevents != 0 && (m_events_emitted == last_evt_nr)) {
            // We exit early (and recycle) because we hit our jana:nevents limit
            DoClose(false);
            return Result::FailureFinished;
        }
        // If we reach this point, we will need to actually read an event

        // We configure the event
        event->SetEventNumber(m_events_emitted); // Default event number to event count
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
            m_events_emitted += 1; 
            // We end up here if we read an entry in our file or retrieved a message from our socket,
            // and believe we could obtain another one immediately if we wanted to
            for (auto* output : m_outputs) {
                output->InsertCollection(*event);
            }
            if (m_events_emitted <= first_evt_nr) {
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

JEventSource::Result JEventSource::DoNextCompatibility(std::shared_ptr<JEvent> event) {

    auto first_evt_nr = m_nskip;
    auto last_evt_nr = m_nevents + m_nskip;

    try {
        if (m_status == Status::Initialized) {
            DoOpen(false);
        }
        if (m_status == Status::Opened) {
            if (m_events_emitted < first_evt_nr) {
                // Skip these events due to nskip
                event->SetEventNumber(m_events_emitted); // Default event number to event count
                auto previous_origin = event->GetJCallGraphRecorder()->SetInsertDataOrigin( JCallGraphRecorder::ORIGIN_FROM_SOURCE);  // (see note at top of JCallGraphRecorder.h)
                GetEvent(event);
                event->GetJCallGraphRecorder()->SetInsertDataOrigin( previous_origin );
                m_events_emitted += 1;
                return Result::FailureTryAgain;  // Reject this event and recycle it
            } else if (m_nevents != 0 && (m_events_emitted == last_evt_nr)) {
                // Declare ourselves finished due to nevents
                DoClose(false); // Close out the event source as soon as it declares itself finished
                return Result::FailureFinished;
            } else {
                // Actually emit an event.
                // GetEvent() expects the following things from its incoming JEvent
                event->SetEventNumber(m_events_emitted);
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
                m_events_emitted += 1;
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


void JEventSource::DoFinish(JEvent& event) {

    m_events_finished.fetch_add(1);
    if (m_enable_finish_event) {
        std::lock_guard<std::mutex> lock(m_mutex);
        CallWithJExceptionWrapper("JEventSource::FinishEvent", [&](){
            FinishEvent(event);
        });
    }
}

void JEventSource::Summarize(JComponentSummary& summary) const {

    auto* result = new JComponentSummary::Component(
        "Source", GetPrefix(), GetTypeName(), GetLevel(), GetPluginName());

    for (const auto* output : m_outputs) {
        size_t suboutput_count = output->collection_names.size();
        for (size_t i=0; i<suboutput_count; ++i) {
            result->AddOutput(new JComponentSummary::Collection("", output->collection_names[i], output->type_name, GetLevel()));
        }
    }

    summary.Add(result);
}
