#include <JANA/JEventSource.h>

void JEventSource::DoInit() {
    std::lock_guard<std::mutex> lock(m_mutex);
    if (m_status != Status::Uninitialized) {
        throw JException("Attempted to initialize a JEventSource that is already initialized!");
    }
    for (auto* parameter : m_parameters) {
        parameter->Init(*(m_app->GetJParameterManager()), m_prefix);
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
    if (m_status == Status::Initialized) {
        DoOpen(false);
    }
    if (m_status == Status::Opened) {

        // First we check whether there are events to skip. If so, we skip as many as possible
        if (m_nskip > 0) {
            auto [result, remaining_events] = Skip(*event.get(), m_nskip);
            m_events_skipped += (m_nskip - remaining_events);
            m_nskip = remaining_events;

            LOG_DEBUG(GetLogger()) << "Finished with Skip: " << m_events_skipped << " events skipped, " << m_nskip << " events to skip remain";

            // If we encountered a problem, exit and let the arrow figure out when and whether to resume.
            // Note that Skip() will call Close() on our behalf.
            if (result != Result::Success) return result;
        }

        // Next we check whether we are limited by jana:nevents
        if (m_nevents != 0 && m_events_emitted >= m_nevents) {
            LOG_DEBUG(GetLogger()) << "Closing EventSource due to reaching nevent limit";
            DoClose(false);
            return Result::FailureFinished;
        }

        // By this point we know that we are done skipping events and are ready to emit them

        // We configure the event
        event->SetEventNumber(m_events_emitted); // Default event number to event count
        event->SetJEventSource(this);
        event->SetSequential(false);
        event->GetJCallGraphRecorder()->Reset();


        JEventSource::Result result = Result::Success;
        try {
            auto previous_origin = event->GetJCallGraphRecorder()->SetInsertDataOrigin( JCallGraphRecorder::ORIGIN_FROM_SOURCE);  // (see note at top of JCallGraphRecorder.h)
            if (m_callback_style == CallbackStyle::LegacyMode) {
                GetEvent(event);
            }
            else {
                result = Emit(*event);
            }
            event->GetJCallGraphRecorder()->SetInsertDataOrigin( previous_origin );
        }
        catch (RETURN_STATUS rs) {

            if (rs == RETURN_STATUS::kNO_MORE_EVENTS) {
                DoClose(false);
                result = Result::FailureFinished;
            }
            else if (rs == RETURN_STATUS::kTRY_AGAIN || rs == RETURN_STATUS::kBUSY) {
                result = Result::FailureTryAgain;
            }
            else if (rs == RETURN_STATUS::kERROR || rs == RETURN_STATUS::kUNKNOWN) {
                JException ex ("JEventSource threw RETURN_STATUS::kERROR or kUNKNOWN");
                ex.plugin_name = m_plugin_name;
                ex.type_name = m_type_name;
                ex.function_name = "JEventSource::GetEvent";
                ex.instance_name = m_resource_name;
                throw ex;
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

        if (result == Result::Success) {
            m_events_emitted += 1;
            // We end up here if we read an entry in our file or retrieved a message from our socket,
            // and believe we could obtain another one immediately if we wanted to
            for (auto* output : m_outputs) {
                output->InsertCollection(*event);
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


void JEventSource::DoFinishEvent(JEvent& event) {

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


std::pair<JEventSource::Result, size_t> JEventSource::Skip(JEvent& event, size_t events_to_skip) {

    // Return values
    Result result = Result::Success;
    size_t events_skipped = 0;

    while (events_to_skip > 0 && result == Result::Success) {
        try {
            auto previous_origin = event.GetJCallGraphRecorder()->SetInsertDataOrigin( JCallGraphRecorder::ORIGIN_FROM_SOURCE);  // (see note at top of JCallGraphRecorder.h)
            if (m_callback_style == CallbackStyle::LegacyMode) {
                GetEvent(event.shared_from_this());
            }
            else {
                result = Emit(event);
            }
            event.GetJCallGraphRecorder()->SetInsertDataOrigin( previous_origin );
            // We need to call FinishEvent, but we don't want to call it from inside JEvent::Clear() because we are already inside the lock.
            // So instead, we call it ourselves out here. This has the added benefit of letting us avoid updating m_events_finished.
            if (m_enable_finish_event) {
                CallWithJExceptionWrapper("JEventSource::FinishEvent", [&](){ FinishEvent(event); });
            }
            event.Clear(false);
            events_skipped += 1;
            events_to_skip -= 1;
            m_events_emitted += 1; // For now skipped events count towards our emit count, but not our finished count.
        }
        catch (RETURN_STATUS rs) {

            if (rs == RETURN_STATUS::kNO_MORE_EVENTS) {
                DoClose(false);
                result = Result::FailureFinished;
            }
            else if (rs == RETURN_STATUS::kTRY_AGAIN || rs == RETURN_STATUS::kBUSY) {
                result = Result::FailureTryAgain;
            }
            else if (rs == RETURN_STATUS::kERROR || rs == RETURN_STATUS::kUNKNOWN) {
                JException ex ("JEventSource threw RETURN_STATUS::kERROR or kUNKNOWN");
                ex.plugin_name = m_plugin_name;
                ex.type_name = m_type_name;
                ex.function_name = (m_callback_style == CallbackStyle::LegacyMode) ? "JEventSource::GetEvent" : "JEventSource::Emit";
                ex.instance_name = m_resource_name;
                throw ex;
            }
        }
        catch (JException& ex) {
            if (ex.function_name.empty()) ex.function_name = (m_callback_style == CallbackStyle::LegacyMode) ? "JEventSource::GetEvent" : "JEventSource::Emit";
            if (ex.type_name.empty()) ex.type_name = m_type_name;
            if (ex.instance_name.empty()) ex.instance_name = m_prefix;
            if (ex.plugin_name.empty()) ex.plugin_name = m_plugin_name;
            throw ex;
        }
        catch (std::exception& e){
            auto ex = JException(e.what());
            ex.exception_type = JTypeInfo::demangle_current_exception_type();
            ex.nested_exception = std::current_exception();
            ex.function_name = (m_callback_style == CallbackStyle::LegacyMode) ? "JEventSource::GetEvent" : "JEventSource::Emit";
            ex.type_name = m_type_name;
            ex.instance_name = m_prefix;
            ex.plugin_name = m_plugin_name;
            throw ex;
        }
        catch (...) {
            auto ex = JException("Unknown exception");
            ex.exception_type = JTypeInfo::demangle_current_exception_type();
            ex.nested_exception = std::current_exception();
            ex.function_name = (m_callback_style == CallbackStyle::LegacyMode) ? "JEventSource::GetEvent" : "JEventSource::Emit";
            ex.type_name = m_type_name;
            ex.instance_name = m_prefix;
            ex.plugin_name = m_plugin_name;
            throw ex;
        }
    }

    return {result, events_to_skip};
}




