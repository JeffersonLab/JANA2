
// Copyright 2023, Jefferson Science Associates, LLC.
// Subject to the terms in the LICENSE file found in the top-level directory.


#ifndef JANA2_JEVENTCALLBACK_H
#define JANA2_JEVENTCALLBACK_H

#include <memory>
#include <mutex>

class JEvent;
namespace jana2 {

class JEventCallback {

private:
    std::once_flag m_is_initialized;
    std::once_flag m_is_finished;
    int32_t m_last_run_number = -1;

public:
    virtual void Init() {}
    virtual void BeginRun(const std::shared_ptr<const JEvent>&) {}
    virtual void Process(const std::shared_ptr<const JEvent>&) {}
    virtual void EndRun() {}
    virtual void Finish() {}

    void Execute(const std::shared_ptr<const JEvent>& event);
    void Release();

};

} // jana2

#endif //JANA2_JEVENTCALLBACK_H
