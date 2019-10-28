//
//    File: streamDet/streamDet.cc
// Created: Wed Apr 24 16:04:14 EDT 2019
// Creator: pooser (on Linux rudy.jlab.org 3.10.0-957.10.1.el7.x86_64 x86_64)
//

#include <JANA/Streaming/JStreamingEventSource.h>
#include <JANA/JCsvWriter.h>
#include <JANA/JEventSourceGeneratorT.h>

#include "RootProcessor.h"
#include "MonitoringProcessor.h"
#include "JFactoryGenerator_streamDet.h"
#include "DecodeDASSource.h"
#include "ADCSampleFactory.h"
#include "INDRAMessage.h"
#include "ZmqTransport.h"

void dummy_publisher_loop() {

    size_t delay_ms = 200;

    std::this_thread::sleep_for(std::chrono::seconds(4));  // Wait for JANA to fire up so we don't lose data
    std::cout << "Starting producer loop" << std::endl;

    ZmqTransport transport {japp->GetParameterValue<std::string>("streamDet:sub_socket"), true};
    transport.initialize();

    DASEventMessage message(japp);
    // INDRAMessage* indra_message = message.as_indra_message();

    size_t current_event_number = 1;

    FILE* f = fopen(japp->GetParameterValue<std::string>("streamDet:data_file").c_str(), "r");
    std::cout << "streamDet::dummy_publisher_loop -> Reading data from data file "
              << japp->GetParameterValue<std::string>("streamDet:data_file").c_str() << std::endl;
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
        message.set_payload_size(static_cast <uint32_t> (payload_capacity));
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

    // TODO: Consider making streamDet:sub_socket be the 'source_name', and use JESG to switch between JSES and DecodeDASSource
    // TODO: Improve parametermanager interface

    bool use_zmq = true;
    bool use_dummy_publisher = false;
    size_t nchannels = 80;
    size_t nsamples = 1024;
    size_t msg_print_freq = 10;
    std::string sub_socket_name = "tcp://127.0.0.1:5556";
    std::string pub_socket_name = "tcp://127.0.0.1:5557";
    std::string data_file_name = "run-5-mhz-80-chan-100-ev.dat";

    std::cout << "Subscribing to INDRA messages via ZMQ on socket " << sub_socket_name << std::endl;
    std::cout << "Publishing JObjects via ZMQ on socket " << pub_socket_name << std::endl;

    app->GetJParameterManager()->SetDefaultParameter("streamDet:use_zmq", use_zmq);
    app->GetJParameterManager()->SetDefaultParameter("streamDet:data_file", data_file_name);
    app->GetJParameterManager()->SetDefaultParameter("streamDet:use_dummy_publisher", use_dummy_publisher);
    app->GetJParameterManager()->SetDefaultParameter("streamDet:nchannels", nchannels);
    app->GetJParameterManager()->SetDefaultParameter("streamDet:nsamples", nsamples);
    app->GetJParameterManager()->SetDefaultParameter("streamDet:msg_print_freq", msg_print_freq);
    app->GetJParameterManager()->SetDefaultParameter("streamDet:sub_socket", sub_socket_name);
    app->GetJParameterManager()->SetDefaultParameter("streamDet:pub_socket", pub_socket_name);

    if (use_zmq) {
        auto transport = std::unique_ptr<ZmqTransport>(new ZmqTransport(sub_socket_name));
        app->Add(new JStreamingEventSource<DASEventMessage>(std::move(transport)));
        if (use_dummy_publisher) {
            new std::thread(dummy_publisher_loop);
        }
    }
    else {
        app->Add(app->GetParameterValue<std::string>("streamDet:data_file"));
        app->Add(new JEventSourceGeneratorT<DecodeDASSource>());
        app->Add(new JFactoryGenerator_streamDet());
    }

    app->Add(new RootProcessor());
    app->Add(new MonitoringProcessor());
    app->Add(new JCsvWriter<ADCSample>());
    app->Add(new JFactoryGeneratorT<ADCSampleFactory>());

}

} // "C"


