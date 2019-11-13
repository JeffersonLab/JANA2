//
//    File: streamDet/MonitoringProcessor.cc
// Created: Wed Apr 24 16:04:14 EDT 2019
// Creator: pooser (on Linux rudy.jlab.org 3.10.0-957.10.1.el7.x86_64 x86_64)
//

#include "MonitoringProcessor.h"
#include "ADCSample.h"

//---------------------------------
// MonitoringProcessor    (Constructor)
//---------------------------------
MonitoringProcessor::MonitoringProcessor() = default;

//---------------------------------
// MonitoringProcessor (Destructor)
//---------------------------------
MonitoringProcessor::~MonitoringProcessor() {
    delete m_transport;
    delete m_message;
}

//------------------
// Init
//------------------
void MonitoringProcessor::Init() {

    // This is called once at program startup.
    // initialize the message and the transport publisher
    m_message   = new DASEventMessage(japp);
    m_transport = new ZmqTransport(japp->GetParameterValue<std::string>("streamDet:pub_socket"), true);
    m_transport->initialize();
    std::cout << "MonitoringProccessor::Init -> Initializing ZMQ sink on socket "
              <<  japp->GetParameterValue<std::string>("streamDet:pub_socket") << std::endl;
}

//------------------
// Process
//------------------
void MonitoringProcessor::Process(const std::shared_ptr<const JEvent>& aEvent) {

    auto oriMessage = aEvent->GetSingle<DASEventMessage>();
    std::lock_guard<std::mutex> lck(msgMutex);
    m_transport->send(*oriMessage);
    size_t eventNum = oriMessage->get_event_number();
    size_t buffSize = oriMessage->get_buffer_size();
    size_t msgFreq  = oriMessage->get_message_print_freq();
    if (eventNum % msgFreq == 0) {
        std::cout << "MonitoringProcessor::Process -> Published event " << eventNum
                  << " on socket " << japp->GetParameterValue<std::string>("streamDet:pub_socket")
                  << " with buffer size = " << buffSize << " bytes" << std::endl;
    }
}

//------------------
// Finish
//------------------
void MonitoringProcessor::Finish() {

}
