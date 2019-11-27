//
// Created by Nathan Brei on 2019-11-24.
//

#ifndef JANA2_JEVENTSOURCEBACKEND_H
#define JANA2_JEVENTSOURCEBACKEND_H

#include <string>
#include <atomic>
#include <mutex>

class JEvent;
class JApplication;

namespace jana {
namespace components {


// JEventSourceBackend is an interface which allows us to try out different API versions simultaneously and
// migrate off the old ones more gently.
struct JEventSourceBackend {

public:
    enum class Status {
        Unopened, Opened, Finished
    };
    enum class Result {
        Success, FailureTryAgain, FailureFinished, FailureError
    };

    virtual void open() = 0;

    virtual Result next(JEvent&) = 0;

protected:
    std::string m_plugin_name;
    std::string m_type_name;
    std::string m_resource_name;
    std::once_flag m_init_flag;
    Status m_status;
    JApplication* m_application = nullptr;
    std::atomic_ullong m_event_count;
};

} // namespace components
} // namespace jana

#endif //JANA2_JEVENTSOURCEBACKEND_H
