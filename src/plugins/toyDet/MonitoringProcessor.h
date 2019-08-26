//
//    File: toyDet/MonitoringProcessor.h
// Created: Wed Apr 24 16:04:14 EDT 2019
// Creator: pooser (on Linux rudy.jlab.org 3.10.0-957.10.1.el7.x86_64 x86_64)
//
// ------ Last repository commit info -----
// [ Date ]
// [ Author ]
// [ Source ]
// [ Revision ]

#ifndef _JEventProcessor_toyDet_h_
#define _JEventProcessor_toyDet_h_

#include <mutex>

#include <JANA/JEventProcessor.h>
#include <JANA/JEvent.h>
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

    std::mutex fillMutex;
    ZmqTransport *m_transport = nullptr;
    DASEventMessage *m_message = nullptr;

};

#endif // _JEventProcessor_toyDet_h_

