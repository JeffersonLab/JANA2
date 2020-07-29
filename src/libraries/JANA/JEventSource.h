
// Copyright 2020, Jefferson Science Associates, LLC.
// Subject to the terms in the LICENSE file found in the top-level directory.


#ifndef _JEventSource_h_
#define _JEventSource_h_

#include <JANA/JException.h>
#include <JANA/Utils/JTypeInfo.h>
#include <JANA/JEvent.h>

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

    virtual void Open() {}

    virtual void GetEvent(std::shared_ptr<JEvent>) = 0;

    // Deprecated. Use JEvent::Insert() from inside GetEvent instead.
    virtual bool GetObjects(const std::shared_ptr<const JEvent>& aEvent, JFactory* aFactory) { return false; }



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

    ReturnStatus DoNext(std::shared_ptr<JEvent> event) {

        std::lock_guard<std::mutex> lock(m_mutex); // In general, DoNext must be synchronized.
        auto first_evt_nr = m_nskip;
        auto last_evt_nr = m_nevents + m_nskip;
        try {
            switch (m_status.load()) {

                case SourceStatus::Unopened: DoInitialize(); // Fall-through to Opened afterwards

                case SourceStatus::Opened:

                    if (m_event_count < first_evt_nr) {
                        m_event_count += 1;
                        GetEvent(event);
                        return ReturnStatus::TryAgain;
                    }
                    else if (m_nevents != 0 && (m_event_count == last_evt_nr)) {
                        m_status = SourceStatus::Finished; // TODO: This isn't threadsafe at the moment
                        return ReturnStatus::Finished;
                    }
                    else {
                        GetEvent(event);
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

    /// SetTypeName is intended as a replacement to GetType(), which should be less confusing for the
    /// user. It should be called from the constructor. For convenience, we provide a
    /// NAME_OF_THIS macro so that the user doesn't have to type the class name as a string, which may
    /// get out of sync if automatic refactoring tools are used.

    // Meant to be called by user
    void SetTypeName(std::string type_name) { m_type_name = std::move(type_name); }

    // Meant to be called by user
    void SetFactoryGenerator(JFactoryGenerator* generator) { m_factory_generator = generator; }

    // Meant to be called by JANA
    void SetApplication(JApplication* app) { m_application = app; }

    // Meant to be called by JANA
    void SetPluginName(std::string plugin_name) { m_plugin_name = std::move(plugin_name); };

    // Meant to be called by JANA
    void SetRange(uint64_t nskip, uint64_t nevents) {
        m_nskip = nskip;
        m_nevents = nevents;
    };


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
    std::mutex m_mutex;
};

#endif // _JEventSource_h_

