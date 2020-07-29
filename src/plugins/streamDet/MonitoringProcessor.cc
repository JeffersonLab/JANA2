
// Copyright 2020, Jefferson Science Associates, LLC.
// Subject to the terms in the LICENSE file found in the top-level directory.

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
    // Initialize the message and the transport publisher
    auto app = GetApplication();
    m_message   = new DASEventMessage(app);
    m_pub_socket = app->GetParameterValue<std::string>("streamDet:pub_socket");
    m_transport = new ZmqTransport(m_pub_socket, true);
    m_transport->initialize();
    std::cout << "MonitoringProccessor::Init -> Initialized ZMQ sink on socket " << m_pub_socket << std::endl;
}

//------------------
// Process
//------------------
void MonitoringProcessor::Process(const std::shared_ptr<const JEvent>& aEvent) {

    auto oriMessage = aEvent->GetSingle<DASEventMessage>();
    std::lock_guard<std::mutex> lck(msgMutex);
    auto result = m_transport->send(*oriMessage);
    if (result == JTransport::SUCCESS) {
        std::cout << "MonitoringProcessor::Process: Success sending " << *oriMessage << std::endl;
    }
    else {
        std::cout << "MonitoringProcessor::Process: Failure sending " << *oriMessage << std::endl;
    }
    size_t eventNum = oriMessage->get_event_number();
    size_t buffSize = oriMessage->get_buffer_size();
    size_t msgFreq  = oriMessage->get_message_print_freq();
    if (eventNum % msgFreq == 0) {
        std::cout << "MonitoringProcessor::Process -> Published event " << eventNum
                  << " on socket " << m_pub_socket
                  << " with buffer size = " << buffSize << " bytes" << std::endl;
    }
}

//------------------
// Finish
//------------------
void MonitoringProcessor::Finish() {

}
