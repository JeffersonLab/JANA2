//
//    File: toyDet/toyDet.cc
// Created: Wed Apr 24 16:04:14 EDT 2019
// Creator: pooser (on Linux rudy.jlab.org 3.10.0-957.10.1.el7.x86_64 x86_64)
//
// ------ Last repository commit info -----
// [ Date ]
// [ Author ]
// [ Source ]
// [ Revision ]

#include <JANA/Streaming/JStreamingEventSource.h>
#include <JANA/JCsvWriter.h>
#include <JANA/JEventSourceGeneratorT.h>

#include "JEventProcessor_toyDet.h"
#include "JFactoryGenerator_toyDet.h"
#include "DASFileSource.h"
#include "ADCSampleFactory.h"
#include "INDRAMessage.h"
#include "ZmqTransport.h"

void dummy_publisher_loop() {

    size_t delay_ms = 200;

    std::this_thread::sleep_for(std::chrono::seconds(4));  // Wait for JANA to fire up so we don't lose data
    std::cout << "Starting producer loop" << std::endl;

    ZmqTransport transport {japp->GetParameterValue<std::string>("toydet:socket"), true};
    transport.initialize();

    DASEventMessage message(japp);
    INDRAMessage* indra_message = message.as_indra_message();

    size_t current_event_number = 1;
    size_t bytes_per_event = japp->GetParameterValue<size_t>("toydet:nsamples") * japp->GetParameterValue<size_t>("toydet:nchannels") * 5;

    FILE* f = fopen(japp->GetParameterValue<std::string>("toydet:filename").c_str(), "r");
    if (f == nullptr) {
        std::cout << "Unable to open file, exiting." << std::endl;
        exit(0);
    }

    while (fread(indra_message->payload, 1, bytes_per_event, f) == bytes_per_event) {

        message.set_event_number(current_event_number);
        transport.send(message);
        std::cout << "Send: " << indra_message << " (" << message.get_buffer_size() << " bytes)" << std::endl;
        consume_cpu_ms(delay_ms, 0, false);
    }

    // Send an empty end-of-stream message
    // TODO: Look ahead
    message.set_payload_size(0);
    message.set_end_of_stream();
    transport.send(message);
    std::cout << "Send: end-of-stream" << std::endl;
}

extern "C" {
void InitPlugin(JApplication* app) {

    InitJANAPlugin(app);
    app->SetParameterValue("nthreads", 4);
    app->SetParameterValue("jana:extended_report", false);
    app->SetParameterValue("jana:legacy_mode", 0);

    // TODO: Consider making toydet:socket be the 'source_name', and use JESG to switch between JSES and DASFileSource
    // TODO: Improve parametermanager interface

    app->SetParameterValue("toydet:filename", std::string("run-5-mhz-80-chan-3-ev.dat"));
    app->SetParameterValue("toydet:socket", std::string("tcp://127.0.0.1:5555"));
    app->SetParameterValue("toydet:nsamples", 1024);
    app->SetParameterValue("toydet:nchannels", 80);

    bool use_zmq = true;
    app->GetJParameterManager()->SetDefaultParameter("toydet:use_zmq", use_zmq);

    bool use_dummy_publisher = true;
    app->GetJParameterManager()->SetDefaultParameter("toydet:use_dummy_publisher", use_dummy_publisher);

    if (use_zmq) {
        auto transport = std::unique_ptr<ZmqTransport>(new ZmqTransport(app->GetParameterValue<std::string>("toydet:socket")));
        app->Add(new JStreamingEventSource<DASEventMessage>(std::move(transport)));

        if (use_dummy_publisher) {
            new std::thread(dummy_publisher_loop);
        }
    }
    else {
        app->Add(app->GetParameterValue<std::string>("toydet:filename"));
        app->Add(new JEventSourceGeneratorT<DASFileSource>());
        app->Add(new JFactoryGenerator_toyDet());
    }

    app->Add(new JEventProcessor_toyDet());
    app->Add(new JCsvWriter<ADCSample>());
    app->Add(new JFactoryGeneratorT<ADCSampleFactory>());
}
} // "C"


