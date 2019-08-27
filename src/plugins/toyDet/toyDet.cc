//
//    File: toyDet/toyDet.cc
// Created: Wed Apr 24 16:04:14 EDT 2019
// Creator: pooser (on Linux rudy.jlab.org 3.10.0-957.10.1.el7.x86_64 x86_64)
//

#include <JANA/Streaming/JStreamingEventSource.h>
#include <JANA/JCsvWriter.h>
#include <JANA/JEventSourceGeneratorT.h>

#include "RootProcessor.h"
#include "MonitoringProcessor.h"
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
    // INDRAMessage* indra_message = message.as_indra_message();

    size_t current_event_number = 1;

    FILE* f = fopen(japp->GetParameterValue<std::string>("toydet:filename").c_str(), "r");
    if (f == nullptr) {
        std::cout << "Unable to open file, exiting." << std::endl;
        exit(0);
    }

    char* payload = nullptr;
    size_t payload_capacity;
    size_t payload_length;
    message.as_payload(&payload, &payload_length, &payload_capacity);

    while (fread(payload, 1, payload_capacity, f) == payload_capacity) {
        message.as_indra_message()->source_id = 0;
        message.set_event_number(current_event_number++);
        message.set_payload_size(payload_capacity);
        std::cout << "Send: " << message << " (" << message.get_buffer_size() << " bytes)" << std::endl;
        transport.send(message);
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

    bool use_zmq = true;
    bool use_dummy_publisher = false;
    size_t nchannels = 80;
    size_t nsamples = 1024;
    std::string socket_name = "tcp://127.0.0.1:5556";
    std::string output_socket_name = "tcp://127.0.0.1:5557";
    std::string file_name = "run-5-mhz-80-chan-100-ev.dat";

    app->GetJParameterManager()->SetDefaultParameter("toydet:use_zmq", use_zmq);
    app->GetJParameterManager()->SetDefaultParameter("toydet:socket", socket_name);
    app->GetJParameterManager()->SetDefaultParameter("toydet:output_socket", output_socket_name);
    app->GetJParameterManager()->SetDefaultParameter("toydet:filename", file_name);
    app->GetJParameterManager()->SetDefaultParameter("toydet:nchannels", nchannels);
    app->GetJParameterManager()->SetDefaultParameter("toydet:nsamples", nsamples);
    app->GetJParameterManager()->SetDefaultParameter("toydet:use_dummy_publisher", use_dummy_publisher);

    if (use_zmq) {
        auto transport = std::unique_ptr<ZmqTransport>(new ZmqTransport(socket_name));
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

    app->Add(new RootProcessor());
    app->Add(new MonitoringProcessor());
    app->Add(new JCsvWriter<ADCSample>());
    app->Add(new JFactoryGeneratorT<ADCSampleFactory>());

}

} // "C"


