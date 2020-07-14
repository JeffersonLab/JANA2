
// Copyright 2020, Jefferson Science Associates, LLC.
// Subject to the terms in the LICENSE file found in the top-level directory.

#ifndef _MonitoringProcessor_h_
#define _MonitoringProcessor_h_

#include <mutex>

#include <JANA/JEventProcessor.h>
#include "ZmqTransport.h"
#include "INDRAMessage.h"

class MonitoringProcessor : public JEventProcessor {

public:

    MonitoringProcessor();
    virtual ~MonitoringProcessor();

    virtual void Init(void);
    virtual void Process(const std::shared_ptr<const JEvent> &aEvent);
    virtual void Finish(void);

private:

    std::mutex msgMutex;
    std::string m_pub_socket;
    ZmqTransport *m_transport  = nullptr;
    DASEventMessage *m_message = nullptr;
};

#endif // _MonitoringProcessor_h_

