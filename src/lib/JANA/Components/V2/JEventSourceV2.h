//
// Created by Nathan Brei on 2/4/20.
//

#ifndef JANA2_J_EVENT_SOURCE_V_2_H
#define JANA2_J_EVENT_SOURCE_V_2_H

#include <JANA/Components/JAbstractEventSource.h>

class JEventSourceV2 : public JAbstractEventSource {

public:

    /// The user is supposed to _throw_ RETURN_STATUS::kNO_MORE_EVENTS or kBUSY from GetEvent()
    enum class RETURN_STATUS { kSUCCESS, kNO_MORE_EVENTS, kBUSY, kTRY_AGAIN, kERROR, kUNKNOWN };

    JEventSourceV2(std::string resource_name, JApplication* app=nullptr) {
        m_resource_name = resource_name;
        m_application = app;
    }

    virtual void Open() {}

    virtual void GetEvent(std::shared_ptr<JEvent>) = 0;

    // Deprecated. Use JEvent::Insert() from inside GetEvent instead.
    virtual bool GetObjects(const std::shared_ptr<const JEvent>& aEvent, JFactory* aFactory) { return false; }


    // Everything below here should be in a JEventSourceBackend instead

    void DoInitialize() final {
        try {
            std::call_once(m_init_flag, &JEventSourceV2::Open, this);
            m_status = SourceStatus::Opened;
        }
        catch (JException& ex) {
            ex.plugin_name = m_plugin_name;
            ex.component_name = GetType();
            throw ex;
        }
        catch (...) {
            auto ex = JException("Unknown exception in JEventSourceV2::Open()");
            ex.nested_exception = std::current_exception();
            ex.plugin_name = m_plugin_name;
            ex.component_name = GetType();
            throw ex;
        }
    }

    ReturnStatus DoNext(std::shared_ptr<JEvent> event) final {
        auto first_evt_nr = m_nskip;
        auto last_evt_nr = m_nevents + m_nskip;
        try {
            switch (m_status) {

                case SourceStatus::Unopened: DoInitialize(); // Fall-through to Opened afterwards

                case SourceStatus::Opened:

                    if (m_event_count < first_evt_nr) {
                        m_event_count += 1;
                        GetEvent(std::move(event));  // Throw this event away
                        return ReturnStatus::TryAgain;
                    }
                    else if (m_nevents != 0 && (m_event_count == last_evt_nr)) {
                        m_status = SourceStatus::Finished; // TODO: This isn't threadsafe at the moment
                        return ReturnStatus::Finished;
                    }
                    else {
                        GetEvent(std::move(event));
                        m_event_count += 1;
                        return ReturnStatus::Success;
                    }

                case SourceStatus::Finished: return ReturnStatus::Finished;
            }
        }
        catch (RETURN_STATUS rs) {

            if (rs == RETURN_STATUS::kNO_MORE_EVENTS) {
                m_status = SourceStatus::Finished; // TODO: This isn't threadsafe at the moment
                return ReturnStatus::Finished;
            }
            else if (rs == RETURN_STATUS::kTRY_AGAIN || rs == RETURN_STATUS::kBUSY) {
                return ReturnStatus::TryAgain;
            }
            else if (rs == RETURN_STATUS::kERROR || rs == RETURN_STATUS::kUNKNOWN) {
                JException ex ("JEventSourceV2 threw RETURN_STATUS::kERROR or kUNKNOWN");
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
        catch (...) {
            auto ex = JException("Unknown exception in JEventSourceV2::GetEvent()");
            ex.nested_exception = std::current_exception();
            ex.plugin_name = m_plugin_name;
            ex.component_name = GetType();
            throw ex;
        }
    }

private:
    std::once_flag m_init_flag;

};


#endif //JANA2_J_EVENT_SOURCE_V_2_H
