
// Copyright 2020, Jefferson Science Associates, LLC.
// Subject to the terms in the LICENSE file found in the top-level directory.


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

void dummy_publisher_loop(JApplication* app) {

    size_t delay_ms = 1;
    auto logger = app->GetService<JLoggingService>()->get_logger("dummy_publisher_loop");

    std::this_thread::yield();
    //std::this_thread::sleep_for(std::chrono::milliseconds(10));  // Wait for JANA to fire up so we don't lose data
    LOG_INFO(logger) << "Starting producer loop" << LOG_END;

    ZmqTransport transport {app->GetParameterValue<std::string>("streamDet:sub_socket"), true};
    transport.initialize();

    DASEventMessage message(app);

    size_t current_event_number = 1;

    FILE* f = fopen(app->GetParameterValue<std::string>("streamDet:data_file").c_str(), "r");
    LOG_INFO(logger) << "streamDet::dummy_publisher_loop -> Reading data from file "
              << app->GetParameterValue<std::string>("streamDet:data_file").c_str() << LOG_END;
    if (f == nullptr) {
        LOG_FATAL(logger) << "Unable to open file, exiting." << LOG_END;
        exit(0);
    }

    char* payload = nullptr;
    size_t payload_capacity;
    size_t payload_length;
    message.as_payload(&payload, &payload_length, &payload_capacity);

    while (fread(payload, 1, payload_capacity, f) == payload_capacity) {
        message.as_indra_message()->source_id = 0;
        message.set_event_number(current_event_number++);
        message.set_payload_size(static_cast<uint32_t>(payload_capacity));
        //LOG_DEBUG(logger) << "Send: " << message << " (" << message.get_buffer_size() << " bytes)" << LOG_END;
        std::cout << "dummy_producer_loop: Sending '" << message << "' (" << message.get_buffer_size() << " bytes)" << std::endl;
        transport.send(message);
        consume_cpu_ms(delay_ms, 0, false);
        std::this_thread::yield();
    }

    // Send an empty end-of-stream message
    message.set_payload_size(0);
    message.set_end_of_stream();
    transport.send(message);
    LOG_INFO(logger) << "Send: end-of-stream" << LOG_END;

}

extern "C" {

void InitPlugin(JApplication* app) {

    InitJANAPlugin(app);
    auto logger = app->GetService<JLoggingService>()->get_logger("streamDet");

    app->SetParameterValue("nthreads", 4);
    app->SetParameterValue("jana:extended_report", true);
    app->SetParameterValue("jana:limit_total_events_in_flight", true);
    app->SetParameterValue("jana:event_pool_size", 16);

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

    LOG_INFO(logger) << "Subscribing to INDRA messages via ZMQ on socket " << sub_socket_name << LOG_END;
    LOG_DEBUG(logger) << "Publishing JObjects via ZMQ on socket " << pub_socket_name << LOG_END;

    app->SetDefaultParameter("streamDet:use_zmq", use_zmq);
    app->SetDefaultParameter("streamDet:data_file", data_file_name);
    app->SetDefaultParameter("streamDet:use_dummy_publisher", use_dummy_publisher);
    app->SetDefaultParameter("streamDet:nchannels", nchannels);
    app->SetDefaultParameter("streamDet:nsamples", nsamples);
    app->SetDefaultParameter("streamDet:msg_print_freq", msg_print_freq);
    app->SetDefaultParameter("streamDet:sub_socket", sub_socket_name);
    app->SetDefaultParameter("streamDet:pub_socket", pub_socket_name);

    if (use_zmq) {
        auto transport = std::unique_ptr<ZmqTransport>(new ZmqTransport(sub_socket_name));
        app->Add(new JStreamingEventSource<DASEventMessage>(std::move(transport)));
        if (use_dummy_publisher) {
            new std::thread(dummy_publisher_loop, app);
        }
    }
    else {
        app->Add(app->GetParameterValue<std::string>("streamDet:data_file"));
        app->Add(new JEventSourceGeneratorT<DecodeDASSource>());
        app->Add(new JFactoryGenerator_streamDet());
    }

    app->Add(new RootProcessor());
    app->Add(new MonitoringProcessor());
    //app->Add(new JCsvWriter<ADCSample>());
    app->Add(new JFactoryGeneratorT<ADCSampleFactory>());

}

} // "C"


